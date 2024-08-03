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

#include <string>
#include <iterator>
#include "ffmpeghandler.h"
#include "log.h"
#include "config.h"

FFmpegHandler::FFmpegHandler() : StreamBaseHandler() {
}

FFmpegHandler::~FFmpegHandler() = default;

std::vector<std::string> FFmpegHandler::prepareStreamCmdVideo(const m3u_stream &stream) {
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

std::vector<std::string> FFmpegHandler::prepareStreamCmdAudio(const m3u_stream &stream) {
    // create parameter list
    std::vector<std::string> callStr{
        "ffmpeg", "-hide_banner", "-re", "-y"
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