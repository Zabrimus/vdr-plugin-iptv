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

TinyProcessLib::Process *StreamBaseHandler::streamHandler;

StreamBaseHandler::StreamBaseHandler() {
    streamHandler = nullptr;
}

StreamBaseHandler::~StreamBaseHandler() {
    stop();
}

bool StreamBaseHandler::streamVideo(const m3u_stream &stream) {
    // sanity check
    stop();

    // create parameter list
    std::vector<std::string> callStr = prepareStreamCmdVideo(stream);

    streamHandler = new TinyProcessLib::Process(callStr, "",
                                                [this](const char *bytes, size_t n) {
                                                  debug9("Queue size %ld\n", tsPackets.size());

                                                  std::lock_guard<std::mutex> guard(queueMutex);
                                                  tsPackets.push_back(std::string(bytes, n));
                                                },

                                                [this](const char *bytes, size_t n) {
                                                  // TODO: ffmpeg prints many information on stderr
                                                  //       How to handle this? ignore? filter?

                                                  std::string msg = std::string(bytes, n);
                                                  debug10("Error: %s\n", msg.c_str());
                                                },

                                                true
    );

    /*
    audioUpdate = std::thread(performAudioInfoUpdate, stream);
    audioUpdate.detach();
    */

    return true;
}

bool StreamBaseHandler::isRunning(int &exit_status) {
    return streamHandler->try_get_exit_status(exit_status);
}

bool StreamBaseHandler::streamAudio(const m3u_stream &stream) {
    // sanity check
    stop();

    // create parameter list
    std::vector<std::string> callStr = prepareStreamCmdAudio(stream);

    streamHandler = new TinyProcessLib::Process(callStr, "",
                                                [this](const char *bytes, size_t n) {
                                                  debug9("Add new packets. Current queue size %ld\n", tsPackets.size());

                                                  std::lock_guard<std::mutex> guard(queueMutex);
                                                  tsPackets.push_back(std::string(bytes, n));
                                                },

                                                [this](const char *bytes, size_t n) {
                                                  std::string msg = std::string(bytes, n);

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

    return true;
}

void StreamBaseHandler::stop() {
    if (streamHandler!=nullptr) {
        streamHandler->kill(true);
        streamHandler->get_exit_status();
        delete streamHandler;
        streamHandler = nullptr;
    }

    std::lock_guard<std::mutex> guard(queueMutex);
    std::deque<std::string> empty;
    std::swap(tsPackets, empty);
}

int StreamBaseHandler::popPackets(unsigned char *bufferAddrP, unsigned int bufferLenP) {
    std::lock_guard<std::mutex> guard(queueMutex);
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
