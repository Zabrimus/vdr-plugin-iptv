#include <string>
#include <chrono>
#include <iterator>
#include "ffmpeghandler.h"
#include "log.h"

FFmpegHandler::FFmpegHandler() {
    streamHandler = nullptr;
}

FFmpegHandler::~FFmpegHandler() {
    stop();
}

std::vector<std::string> FFmpegHandler::prepareStreamCmdVideo(const m3u_stream& stream) {
    // create parameter list
    std::vector<std::string> callStr {
            "ffmpeg", "-hide_banner", "-re", "-y"
    };

    // add main input
    callStr.emplace_back("-i");
    callStr.emplace_back(stream.url);

    // add optional audio input
    std::vector<std::string> audioMetadata;
    for (auto a : stream.audio) {
        callStr.emplace_back("-i");
        callStr.emplace_back(a.uri);
    }

    // transmux
    callStr.emplace_back("-codec");
    callStr.emplace_back("copy");

    // main input
    if (!stream.audio.empty()) {
        callStr.emplace_back("-map");
        callStr.emplace_back("0:v");

        int idx = 1;
        for (auto a: stream.audio) {
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
    for (auto a: stream.apids) {
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

    callStr.emplace_back("-mpegts_flags");
    callStr.emplace_back("nit");

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

std::vector<std::string> FFmpegHandler::prepareStreamCmdAudio(const m3u_stream& stream) {
    // create parameter list
    std::vector<std::string> callStr {
            "ffmpeg", "-hide_banner", "-re", "-y"
    };

    // add main input
    callStr.emplace_back("-i");
    callStr.emplace_back(stream.url);

    // add optional audio input
    std::vector<std::string> audioMetadata;
    for (auto a : stream.audio) {
        callStr.emplace_back("-i");
        callStr.emplace_back(a.uri);
    }

    // transmux
    callStr.emplace_back("-codec");
    callStr.emplace_back("copy");

    // main input
    if (!stream.audio.empty()) {
        int idx = 0;
        for (auto a: stream.audio) {
            callStr.emplace_back("-map");
            callStr.emplace_back(std::to_string(idx) + ":a");
            idx++;
        }

        // TODO: Metadata einfügen, sofern vorhanden
        callStr.insert(std::end(callStr), std::begin(audioMetadata), std::end(audioMetadata));
    }

    int aidx = 0;
    for (auto a: stream.apids) {
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

    callStr.emplace_back("-mpegts_flags");
    callStr.emplace_back("nit");

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


bool FFmpegHandler::streamVideo(const m3u_stream& stream) {
    // create parameter list
    std::vector<std::string> callStr = prepareStreamCmdVideo(stream);

    streamHandler = new TinyProcessLib::Process(callStr, "",
        [this](const char *bytes, size_t n) {
            debug9("Queue size %ld\n", tsPackets.size());

            std::lock_guard<std::mutex> guard(queueMutex);
            tsPackets.emplace(bytes, n);
        },

        [this](const char *bytes, size_t n) {
            // TODO: ffmpeg prints many information on stderr
            //       How to handle this? ignore? filter?

            std::string msg = std::string(bytes, n);
            debug10("Error: %s\n", msg.c_str());
        },

        true
    );

    return true;
}

bool FFmpegHandler::streamAudio(const m3u_stream& stream) {
    // create parameter list
    std::vector<std::string> callStr = prepareStreamCmdAudio(stream);

    streamHandler = new TinyProcessLib::Process(callStr, "",
            [this](const char *bytes, size_t n) {
                debug9("Queue size %ld\n", tsPackets.size());

                std::lock_guard<std::mutex> guard(queueMutex);
                tsPackets.emplace(bytes, n);
            },

            [this](const char *bytes, size_t n) {
                // TODO: ffmpeg prints many information on stderr
                //       How to handle this? ignore? filter?

                std::string msg = std::string(bytes, n);
                debug10("Error: %s\n", msg.c_str());
            },

            true
    );

    return true;
}

void FFmpegHandler::stop() {
    if (streamHandler != nullptr) {
        streamHandler->kill(true);
        streamHandler->get_exit_status();
        delete streamHandler;
        streamHandler = nullptr;
    }

    std::queue<std::string> empty;
    std::swap(tsPackets, empty);
}

int FFmpegHandler::popPackets(unsigned char* bufferAddrP, unsigned int bufferLenP) {
    std::lock_guard<std::mutex> guard(queueMutex);
    if (!tsPackets.empty()) {
        std::string front = tsPackets.front();

        if (bufferLenP < front.size()) {
            error("WARNING: BufferLen %u < Size %ld\n", bufferLenP, front.size());
            return -1;
        }

        debug9("Read from queue, size %ld\n", front.size());

        memcpy(bufferAddrP, front.data(), front.size());

        tsPackets.pop();
        return front.size();
    }

    return -1;
}