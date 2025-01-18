#include <string>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <sys/stat.h>
#include <vdr/plugin.h>
#include <iterator>
#include "config.h"
#include "streambasehandler.h"
#include "log.h"


/**
 * Example of the ffmpeg call. Just for information and better understanding.
 *
 * ffmpeg -hide_banner -re -y
 *          -thread_queue_size 32
 *          -i https://kikageohls.akamaized.net/hls/live/2022693/livetvkika_de/master-1080p-5000.m3u8
 *          -thread_queue_size 32
 *          -i https://kikageohls.akamaized.net/hls/live/2022693/livetvkika_de/master-audio-01u02-st.m3u8
 *          -thread_queue_size 32
 *          -i https://kikageohls.akamaized.net/hls/live/2022693/livetvkika_de/master-audio-05u06-ad.m3u8
 *          -thread_queue_size 32
 *          -i https://kikageohls.akamaized.net/hls/live/2022693/livetvkika_de/master-audio-07u08-ks.m3u8
 *          -codec copy
 *          -c:s copy
 *          -map 0:v
 *          -map 1:a
 *          -map 2:a
 *          -map 3:a
 *          -streamid 0:410
 *          -streamid 1:411
 *          -streamid 2:412
 *          -streamid 3:413
 *          -f mpegts
 *          -mpegts_transport_stream_id 1
 *          -mpegts_pmt_start_pid 4096
 *          -mpegts_service_id 22
 *          -mpegts_original_network_id 65281
 *          -mpegts_flags system_b
 *          -mpegts_flags nit
 *          -metadata service_name=KiKA
 *          pipe:1
 */

/**
 * Example of the vlc call. Just for information and better understanding.
 *
 * vlc -I dummy -v \
 *     --network-caching=4000 \
 *     --live-caching 2000 \
 *     --http-reconnect \
 *     --http-user-agent=Mozilla/5.0 \
 *     --adaptive-logic highest https://kikageohls.akamaized.net/hls/live/2022693/livetvkika_de/master.m3u8 \
 *     --sout "#standard{access=file,mux=ts{use-key-frames,pid-video=100,pid-audio=200,pid-spu=4096,tsid=2850},dst=-}"
 *
 */


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

// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template<typename ... Args> std::string string_format(const std::string& format, Args ... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;

    if( size_s <= 0 ) {
        throw std::runtime_error( "Error during formatting." );
    }

    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);

    return std::string( buf.get(), buf.get() + size - 1 );
}

void killPid(int pid, int signal) {
    std::string procf("/proc/");
    procf.append(std::to_string(pid));

    struct stat sts{};
    if (!(stat(procf.c_str(), &sts) == -1 && errno == ENOENT)) {
        kill(-pid, signal);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        kill(pid, signal);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!(stat(procf.c_str(), &sts) == -1 && errno == ENOENT)) {
        // must not happen
        // printf("Prozess %d läuft noch\n", pid);
    }
}

StreamBaseHandler::StreamBaseHandler(std::string type) : type(type) {
    debug1("%s (%s)", __PRETTY_FUNCTION__, type.c_str());

    streamHandler = nullptr;
    streamThreadRunning.store(false);
}

StreamBaseHandler::~StreamBaseHandler() {
    stop();
}

void StreamBaseHandler::setChannelId(int channelId) {
    this->channelId = channelId;
}

void StreamBaseHandler::setHandlerType(char handlerType) {
    this->handlerType = handlerType;
}

std::string StreamBaseHandler::convertStreamToJson(bool hasVideo, const m3u_stream &stream) {
    // create json object and call script to get command line
    std::string json;

    json.append("{");
    json.append("\"threads\": ").append(std::to_string(IptvConfig.GetThreadQueueSize())).append(",");

    if (hasVideo) {
        json.append("\"width\": ").append(std::to_string(stream.width)).append(",");
        json.append("\"height\": ").append(std::to_string(stream.height)).append(",");
    }

    json.append("\"url\": \"").append(stream.url).append("\",");

    if (!stream.audio.empty()) {
        json.append("\"audiourl\": [");

        for (long unsigned int i = 0; i < stream.audio.size(); ++i) {
            json.append("\"").append(stream.audio[i].uri).append("\"");
            if (i < stream.audio.size() - 1) {
                json.append(",");
            }
        }

        json.append("],");
    }

    json.append("\"channelName\": \"").append(stream.channelName).append("\",");

    if (hasVideo) {
        json.append("\"vpid\": ").append(std::to_string(stream.vpid)).append(",");
        json.append("\"apid\": [");

        int aidx = 1;
        int maxPid = 0;
        for (auto i : stream.apids) {
            if (i > maxPid) {
                maxPid = i;
            }
        }

        // add well known pid
        for (unsigned long i = 0; i < min(stream.apids.size(), stream.audio.size()); ++i) {
            json.append(std::to_string(stream.apids[i])).append(",");
            aidx++;
        }

        // add missing pid
        int naidx = 1;
        for (auto i = stream.apids.size(); i < stream.audio.size(); ++i) {
            json.append(std::to_string(maxPid + naidx)).append(",");
            aidx++;
            naidx++;
        }

        if (aidx == 1) {
            // no audio channels found, add default
            json.append(std::to_string(stream.vpid + 1));
        }

        // json = json.substr(0, json.length()-1);
        json.append("],");
    } else {
        int apid = stream.apids.empty() ? 257 : stream.apids[0];
        json.append("\"apid\": [");
        json.append(std::to_string(apid));
        json.append("],");
    }

    json.append("\"spid\": ").append(std::to_string(stream.spid)).append(",");
    json.append("\"tpid\": ").append(std::to_string(stream.tpid)).append(",");
    json.append("\"nid\": ").append(std::to_string(stream.nid));

    json.append("}");

    return json;
}

void StreamBaseHandler::streamVideoInternal(const m3u_stream &stream) {
    streamThreadRunning.store(true);

    // create parameter list
    std::vector<std::string> callStrVec;
    std::string callStr;

    bool cmdIsVector = true;

    if (handlerType == 'V') {
        callStrVec = prepareStreamCmdVideoVlc(stream);
    } else if (handlerType == 'F') {
        callStrVec = prepareStreamCmdVideoFfmpeg(stream);
    } else if (handlerType == 'E') {
        cmdIsVector = false;

        // create json object and call script to get command line
        std::string json = convertStreamToJson(true, stream);
        callStr = prepareExpertCmdLine(1, json);

        debug1("CmdLine: %s", callStr.c_str());
    } else {
        streamThreadRunning.store(false);
        // TODO: Error message
        return;
    }

    if (cmdIsVector) {
        streamHandler = new TinyProcessLib::Process(callStrVec, "",
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
    } else {
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
    }

    streamHandler->get_exit_status();
    delete streamHandler;
    streamHandler = nullptr;

    streamThreadRunning.store(false);
}

void StreamBaseHandler::streamAudioInternal(const m3u_stream &stream) {
    streamThreadRunning.store(true);

    bool cmdIsVector = true;

    // create parameter list
    std::vector<std::string> callStrVec;
    std::string callStr;

    if (handlerType == 'V') {
        callStrVec = prepareStreamCmdAudioVlc(stream);
    } else if (handlerType == 'F') {
        callStrVec = prepareStreamCmdAudioFfmpeg(stream);
    } else if (handlerType == 'E') {
        cmdIsVector = false;

        // create json object and call script to get command line
        std::string json = convertStreamToJson(false, stream);
        callStr = prepareExpertCmdLine(0, json);

        debug1("CmdLine: %s", callStr.c_str());
    } else {
        streamThreadRunning.store(false);
        // TODO: Error message
        return;
    }

    if (cmdIsVector) {
        streamHandler = new TinyProcessLib::Process(callStrVec, "",
                                                    [this](const char *bytes, size_t n) {
                                                        debug9("Add new packets. Current queue size %ld\n",
                                                               tsPackets.size());

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
    } else {
        streamHandler = new TinyProcessLib::Process(callStr, "",
                                                    [this](const char *bytes, size_t n) {
                                                        debug9("Add new packets. Current queue size %ld\n",
                                                               tsPackets.size());

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
    }

    streamHandler->get_exit_status();
    delete streamHandler;
    streamHandler = nullptr;

    streamThreadRunning.store(false);
}

void StreamBaseHandler::checkErrorOut(const std::string &msg) {
    bool is404 = false;

    // vlc
    if (msg.find("status: \"404\"") != std::string::npos || msg.find("status: \"400\"") != std::string::npos) {
        cString errmsg = cString::sprintf(tr("Unable to load stream"));
        debug1("%s", *errmsg);

        Skins.Message(mtError, errmsg);

        // TODO: Hier müsste alles gestoppt werden. Allerdings funktioniert diese Variante nicht,
        //  da ein deadlock erzeugt wird.
        // stop();

        is404 = true;
    }

    // ffmpeg
    if (msg.find("HTTP error 404") != std::string::npos || msg.find("HTTP error 400") != std::string::npos) {
        cString errmsg = cString::sprintf(tr("Unable to load stream"));
        debug1("%s", *errmsg);

        Skins.Message(mtError, errmsg);

        // TODO: Hier müsste alles gestoppt werden. Allerdings funktioniert diese Variante nicht,
        //  da ein deadlock erzeugt wird.
        // stop();

        is404 = true;
    }

    if (is404) {
        mark404Channel(channelId);
    }
}

bool StreamBaseHandler::streamVideo(const m3u_stream &stream) {
    if (streamThreadRunning) {
        stop();
    }

    if (streamThread.joinable()) {
        streamThread.join();
    }

    streamThread = std::thread(&StreamBaseHandler::streamVideoInternal, this, stream);
    return true;
}

bool StreamBaseHandler::streamAudio(const m3u_stream &stream) {
    if (streamThreadRunning) {
        stop();
    }

    if (streamThread.joinable()) {
        streamThread.join();
    }

    streamThread = std::thread(&StreamBaseHandler::streamAudioInternal, this, stream);
    return true;
}

void StreamBaseHandler::stop() {
    if ((streamThreadRunning && streamHandler ==nullptr) || (!streamThreadRunning && streamHandler != nullptr)) {
        printBacktrace();
    }

    if (streamHandler != nullptr) {
        int pid = streamHandler->get_id();
        killPid(pid, SIGKILL);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        killPid(pid, SIGTERM);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // streamHandler->kill(true);

        streamHandler = nullptr;
    }

    if (streamThread.joinable()) {
        streamThread.join();
        streamThreadRunning.store(false);
    }

    delete streamHandler;
    streamHandler = nullptr;

    std::lock_guard<std::mutex> guard(queueMutex);
    std::deque<std::string> empty;
    std::swap(tsPackets, empty);
}

int StreamBaseHandler::popPackets(unsigned char *bufferAddrP, unsigned int bufferLenP) {
    if (streamHandler == nullptr) {
        return 0;
    }

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


std::vector<std::string> StreamBaseHandler::prepareStreamCmdVideoFfmpeg(const m3u_stream &stream) {
    // create parameter list
    std::vector<std::string> callStr{
        "ffmpeg", "-hide_banner", "-re", "-y"
    };

    // add main input
    callStr.emplace_back("-thread_queue_size");
    callStr.emplace_back(std::to_string(IptvConfig.GetThreadQueueSize()));
    callStr.emplace_back("-i");
    callStr.emplace_back(stream.url);

    // add optional audio input
    std::vector<std::string> audioMetadata;
    for (const auto& a : stream.audio) {
        callStr.emplace_back("-thread_queue_size");
        callStr.emplace_back(std::to_string(IptvConfig.GetThreadQueueSize()));
        callStr.emplace_back("-i");
        callStr.emplace_back(a.uri);
    }

    // transmux
    callStr.emplace_back("-codec");
    callStr.emplace_back("copy");

    // copy subtitles
    // callStr.emplace_back("-c:s");
    // callStr.emplace_back("copy");

    // main input
    if (!stream.audio.empty()) {
        callStr.emplace_back("-map");
        callStr.emplace_back("0:v");

        int idx = 1;
        for (auto a : stream.audio) {
            callStr.emplace_back("-map");
            callStr.emplace_back(std::to_string(idx) + ":a");
            idx++;
        }

        // TODO: Metadata einfügen, sofern vorhanden
        callStr.insert(std::end(callStr), std::begin(audioMetadata), std::end(audioMetadata));
    }

    callStr.emplace_back("-streamid");
    callStr.emplace_back("0:" + std::to_string(stream.vpid));

    int aidx = 1;
    int maxPid = 0;
    for (auto i : stream.apids) {
        if (i > maxPid) {
            maxPid = i;
        }
    }

    // add well known pid
    for (unsigned long i = 0; i < min(stream.apids.size(), stream.audio.size()); ++i) {
        callStr.emplace_back("-streamid");
        callStr.emplace_back(std::to_string(aidx) + ":" + std::to_string(stream.apids[i]));
        aidx++;
    }

    // add missing pid
    int naidx = 1;
    for (auto i = stream.apids.size(); i < stream.audio.size(); ++i) {
        callStr.emplace_back("-streamid");
        callStr.emplace_back(std::to_string(aidx) + ":" + std::to_string(maxPid + naidx));
        aidx++;
        naidx++;
    }

    callStr.emplace_back("-f");
    callStr.emplace_back("mpegts");

    callStr.emplace_back("-mpegts_transport_stream_id");
    callStr.emplace_back(std::to_string(stream.tpid));

    callStr.emplace_back("-mpegts_pmt_start_pid");
    callStr.emplace_back("4096");

    callStr.emplace_back("-mpegts_service_id");
    callStr.emplace_back(std::to_string(stream.spid));

    callStr.emplace_back("-mpegts_original_network_id");
    callStr.emplace_back(std::to_string(stream.nid));

    callStr.emplace_back("-mpegts_flags");
    callStr.emplace_back("system_b");

    // callStr.emplace_back("-mpegts_flags");
    // callStr.emplace_back("nit");

    callStr.emplace_back("-metadata");
    callStr.emplace_back("service_name=" + stream.channelName);

    callStr.emplace_back("pipe:1");

    std::ostringstream paramOut;
    if (!callStr.empty()) {
        std::copy(callStr.begin(), callStr.end() - 1, std::ostream_iterator<std::string>(paramOut, " "));
        paramOut << callStr.back();
    }

    debug2("FFmpeg call: %s\n", paramOut.str().c_str());

    return callStr;
}

std::vector<std::string> StreamBaseHandler::prepareStreamCmdAudioFfmpeg(const m3u_stream &stream) {
    // create parameter list
    std::vector<std::string> callStr{
        "ffmpeg", "-hide_banner", "-nostats", "-loglevel", "verbose", "-re", "-y"
    };

    // add main input
    callStr.emplace_back("-i");
    callStr.emplace_back(stream.url);

    // add optional audio input
    std::vector<std::string> audioMetadata;
    for (const auto& a : stream.audio) {
        callStr.emplace_back("-i");
        callStr.emplace_back(a.uri);
    }

    // transmux
    callStr.emplace_back("-codec");
    callStr.emplace_back("copy");

    // callStr.emplace_back("-acodec");
    // callStr.emplace_back("libmp3lame");

    // main input
    if (!stream.audio.empty()) {
        int idx = 0;
        for (auto a : stream.audio) {
            callStr.emplace_back("-map");
            callStr.emplace_back(std::to_string(idx) + ":a");
            idx++;
        }

        // TODO: Metadata einfügen, sofern vorhanden
        callStr.insert(std::end(callStr), std::begin(audioMetadata), std::end(audioMetadata));
    }

    int aidx = 0;
    for (auto a : stream.apids) {
        callStr.emplace_back("-streamid");
        callStr.emplace_back(std::to_string(aidx) + ":" + std::to_string(a));
        aidx++;
    }

    callStr.emplace_back("-f");
    callStr.emplace_back("mpegts");

    callStr.emplace_back("-mpegts_transport_stream_id");
    callStr.emplace_back(std::to_string(stream.tpid));

    callStr.emplace_back("-mpegts_pmt_start_pid");
    callStr.emplace_back("4096");

    callStr.emplace_back("-mpegts_service_id");
    callStr.emplace_back(std::to_string(stream.spid));

    callStr.emplace_back("-mpegts_original_network_id");
    callStr.emplace_back("65281");

    callStr.emplace_back("-mpegts_flags");
    callStr.emplace_back("system_b");

    // callStr.emplace_back("-mpegts_flags");
    // callStr.emplace_back("nit");

    callStr.emplace_back("-metadata");
    callStr.emplace_back("service_name=" + stream.channelName);

    callStr.emplace_back("pipe:1");

    std::ostringstream paramOut;
    if (!callStr.empty()) {
        std::copy(callStr.begin(), callStr.end() - 1, std::ostream_iterator<std::string>(paramOut, " "));
        paramOut << callStr.back();
    }

    debug2("FFmpeg call: %s\n", paramOut.str().c_str());

    return callStr;
}

std::vector<std::string> StreamBaseHandler::prepareStreamCmdVideoVlc(const m3u_stream &stream) {
    // create parameter list
    std::vector<std::string> callStr{
        "vlc", "-I", "dummy", "-v",
        "--network-caching=4000", "--live-caching", "2000",
        "--http-reconnect", "--http-user-agent=Mozilla/5.0",
        "--adaptive-logic", "highest"
    };

    callStr.emplace_back(stream.url);
    callStr.emplace_back("--sout");

    int apid = stream.apids.empty() ? 257 : stream.apids[0];

    std::string v = string_format("#standard{access=file,mux=ts{use-key-frames,pid-video=%d,pid-audio=%d,pid-spu=4096,tsid=%d},dst=-}", stream.vpid, apid, stream.tpid);
    callStr.emplace_back(v);

    std::ostringstream paramOut;
    if (!callStr.empty()) {
        std::copy(callStr.begin(), callStr.end() - 1, std::ostream_iterator<std::string>(paramOut, " "));
        paramOut << callStr.back();
    }

    debug2("vlc call: %s\n", paramOut.str().c_str());

    return callStr;
}

std::vector<std::string> StreamBaseHandler::prepareStreamCmdAudioVlc(const m3u_stream &stream) {

    std::vector<std::string> callStr{
        "vlc", "-I", "dummy", "--verbose=2", "--no-stats", stream.url, "--sout"
    };

    int apid = stream.apids.empty() ? 257 : stream.apids[0];
    std::string v = string_format("#transcode{acodec=mpga,ab=320}:standard{access=file,mux=ts{pid-audio=%d,pid-spu=4096,tsid=%d},dst=-}", apid, stream.tpid);
    callStr.emplace_back(v);

    std::ostringstream paramOut;
    if (!callStr.empty()) {
        std::copy(callStr.begin(), callStr.end() - 1, std::ostream_iterator<std::string>(paramOut, " "));
        paramOut << callStr.back();
    }

    debug2("vlc call: %s\n", paramOut.str().c_str());

    return callStr;
}

std::string StreamBaseHandler::prepareExpertCmdLine(int isVideo, const std::string &json) {
    std::vector<std::string> callStr{
            std::string(IptvConfig.GetConfigDirectory()) + "/iptv-cmdline-expert.py",
            std::to_string(isVideo), json
    };

    debug1("Call expert script: %s %s \"%s\"", callStr.at(0).c_str(), callStr.at(1).c_str(), callStr.at(2).c_str());

    std::string cmdLine;
    auto handler = new TinyProcessLib::Process(callStr, "",
                                               [&cmdLine](const char *bytes, size_t n) {
                                                   std::string result = std::string(bytes, n);
                                                   cmdLine = std::string(bytes, n);
                                               },

                                               [](const char *bytes, size_t n) {
                                                   std::string msg = std::string(bytes, n);
                                                   debug1("iptv-cmdline-expert.py Error: %s\n", msg.c_str());
                                               },

                                               true
    );

    int exitStatus = handler->get_exit_status();
    if (exitStatus!=0) {
        debug1("iptv-cmdline-expert.py throws an error, abort\n");
        return {};
    }

    return cmdLine;
}
