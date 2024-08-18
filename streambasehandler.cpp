#include <string>
#include <chrono>
#include <iterator>
#include <vdr/plugin.h>
#include "config.h"
#include "streambasehandler.h"
#include "log.h"

#ifndef TS_ONLY_FULL_PACKETS
#define TS_ONLY_FULL_PACKETS 0
#endif

/* disabled until a better solution is found
std::thread audioUpdate;

void performAudioInfoUpdate(m3u_stream stream) {
    int tries = 10;

    auto device = cDevice::PrimaryDevice();

    int ms = 500;
    while (tries > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));

        for (unsigned long i = 0; i < stream.audio.size(); ++i) {
            device->SetAvailableTrack(eTrackType::ttAudio, i, 0, stream.audio[i].language.c_str(), stream.audio[i].name.c_str());
        }

        tries--;
    }
}
*/

std::atomic<bool> streamThreadRunning(false);

StreamBaseHandler::StreamBaseHandler(int channelId) : channelId(channelId) {
    streamHandler = nullptr;
    streamThreadRunning.store(false);
}

StreamBaseHandler::~StreamBaseHandler() {
    if (streamThread.joinable()) {
        streamThread.join();
        streamThreadRunning.store(false);
    }

    stop();
}

// void StreamBaseHandler::streamVideoInternal(std::vector<std::string> &callStr) {
void StreamBaseHandler::streamVideoInternal(const m3u_stream &stream) {
    streamThreadRunning.store(true);

    // create parameter list
    std::vector<std::string> callStr = prepareStreamCmdVideo(stream);

    streamHandler = new TinyProcessLib::Process(callStr, "",
                                                [this](const char *bytes, size_t n) {
                                                  debug9("Queue size %ld\n", tsPackets.size());

                                                  std::lock_guard<std::mutex> guard(queueMutex);
                                                  tsPackets.push_back(std::string(bytes, n));
                                                },

                                                [this, stream](const char *bytes, size_t n) {
                                                  std::string msg = std::string(bytes, n);
                                                  checkErrorOut(msg);

                                                  debug10("Error: %s\n", msg.c_str());
                                                },

                                                true
    );

    streamHandler->get_exit_status();
    streamThreadRunning.store(false);

    stop();

    // printf("Video Process stopped...\n");
}

void StreamBaseHandler::streamAudioInternal(const m3u_stream &stream) {
    streamThreadRunning.store(true);

    // create parameter list
    std::vector<std::string> callStr = prepareStreamCmdAudio(stream);

    streamHandler = new TinyProcessLib::Process(callStr, "",
                                                [this](const char *bytes, size_t n) {
                                                  debug9("Add new packets. Current queue size %ld\n", tsPackets.size());

                                                  std::lock_guard<std::mutex> guard(queueMutex);
                                                  tsPackets.push_back(std::string(bytes, n));
                                                },

                                                [this, stream](const char *bytes, size_t n) {
                                                  std::string msg = std::string(bytes, n);
                                                  checkErrorOut(msg);

                                                  /*
                                                  size_t idx;

                                                  // fmpeg version
                                                  idx = msg.find("StreamTitle");
                                                  if (idx != std::string::npos) {
                                                      auto idx2 = msg.find(':', idx);
                                                      std::string title = msg.substr(idx2+1);
                                                      trim(title);

                                                      if (!title.empty()) {
                                                         TODO: Wie kommen die Daten in das Radio Plugin?
                                                          auto idx3 = title.find('-');
                                                          if (idx3 != std::string::npos) {
                                                              artist = title.substr(0, idx3);
                                                              title = title.substr(idx3 + 1);
                                                          } else {
                                                              text = title;
                                                          }

                                                          printf("Stream Title: %s\n", title.c_str());
                                                      }
                                                  }

                                                  // vlc version
                                                  idx = msg.find("New Icy-Title");
                                                  if (idx != std::string::npos) {
                                                      auto idx2 = msg.find('=', idx);
                                                      std::string title = msg.substr(idx2+1);
                                                      trim(title);

                                                      if (!title.empty()) {
                                                        TODO: Wie kommen die Daten in das Radio Plugin?
                                                          auto idx3 = title.find('-');
                                                          if (idx3 != std::string::npos) {
                                                              artist = title.substr(0, idx3);
                                                              title = title.substr(idx3 + 1);
                                                          } else {
                                                              text = title;
                                                          }
                                                          printf("Stream Title: %s\n", title.c_str());
                                                      }
                                                  }
                                                  */
                                                  debug10("Error: %s\n", msg.c_str());
                                                },

                                                true
    );

    streamHandler->get_exit_status();
    streamThreadRunning.store(false);

    stop();

    // printf("Audio Process stopped...\n");
}

void StreamBaseHandler::checkErrorOut(const std::string &msg) {
    bool is404 = false;

    // vlc
    if (msg.find("status: \"404\"") != std::string::npos || msg.find("status: \"400\"") != std::string::npos) {
        cString errmsg = cString::sprintf(tr("Unable to load stream"));
        debug1("%s", *errmsg);

        Skins.Message(mtError, errmsg);
        stop();

        is404 = true;
    }

    // ffmpeg
    if (msg.find("HTTP error 404") != std::string::npos || msg.find("HTTP error 400") != std::string::npos) {
        cString errmsg = cString::sprintf(tr("Unable to load stream"));
        debug1("%s", *errmsg);

        Skins.Message(mtError, errmsg);

        is404 = true;
    }

    if (is404) {
        mark404Channel(channelId);
    }
}

bool StreamBaseHandler::streamVideo(const m3u_stream &stream) {
    // sanity check
    stop();

    streamThread = std::thread(&StreamBaseHandler::streamVideoInternal, this, stream);
    return true;
}

bool StreamBaseHandler::streamAudio(const m3u_stream &stream) {
    // sanity check
    stop();

    streamThread = std::thread(&StreamBaseHandler::streamAudioInternal, this, stream);
    return true;
}

void StreamBaseHandler::stop() {
    if (streamHandler != nullptr) {
        if (streamThreadRunning) {
            streamHandler->kill(true);

            if (streamThread.joinable()) {
                streamThread.join();
                streamThreadRunning.store(false);
            }

            if (streamHandler!=nullptr) {
                streamHandler->get_exit_status();
            }
        }

        delete streamHandler;
        streamHandler = nullptr;
    }

    std::lock_guard<std::mutex> guard(queueMutex);
    std::deque<std::string> empty;
    std::swap(tsPackets, empty);
}

int StreamBaseHandler::popPackets(unsigned char *bufferAddrP, unsigned int bufferLenP) {
    std::lock_guard<std::mutex> guard(queueMutex);

    if (streamHandler == nullptr) {
        return 0;
    }

    if (!tsPackets.empty()) {
        std::string front = tsPackets.front();

        if (bufferLenP < front.size()) {
            // this shall never happen. A possible solution is to use a deque instead
            // and modify the front element. Unsure, if this is worth the effort, because
            // a full buffer points to another problem.
            error("WARNING: BufferLen %u < Size %ld\n", bufferLenP, front.size());

            // remove packet from queue to prevent queue overload
            tsPackets.pop_front();

            return 0;
        }

        debug9("Read from queue: len %ld, size %ld bytes\n", tsPackets.size(), front.size());

#if TS_ONLY_FULL_PACKETS == 1
        int full = front.size() / 188;
        int rest = front.size() % 188;
        if (rest != 0) {
            // return only full packets
            memcpy(bufferAddrP, front.data(), full * 188);
            tsPackets.pop_front();
            std::string newfront = front.substr(full*188);

            if (!tsPackets.empty()) {
                newfront.append(tsPackets.front());
                tsPackets.pop_front();
            }
            tsPackets.push_front(newfront);

            return full * 188;
        } else {
            memcpy(bufferAddrP, front.data(), front.size());
            tsPackets.pop_front();
            return front.size();
        }
#else
        memcpy(bufferAddrP, front.data(), front.size());
        tsPackets.pop_front();
        return front.size();
#endif
    }

    return 0;
}
