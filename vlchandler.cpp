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

#include <string>
#include <iterator>
#include "vlchandler.h"
#include "log.h"
#include "config.h"

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

VlcHandler::VlcHandler() : StreamBaseHandler() {
}

VlcHandler::~VlcHandler() = default;

std::vector<std::string> VlcHandler::prepareStreamCmdVideo(const m3u_stream &stream) {
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

std::vector<std::string> VlcHandler::prepareStreamCmdAudio(const m3u_stream &stream) {

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