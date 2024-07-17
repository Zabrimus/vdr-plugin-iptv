#pragma once

#include <string>
#include <thread>
#include <queue>
#include "process.hpp"
#include "m3u8handler.h"

class FFmpegHandler {
private:
    TinyProcessLib::Process *streamHandler;

public:
    FFmpegHandler();
    ~FFmpegHandler();

    bool streamVideo(const m3u_stream& stream);
    void stopVideo();

    int popPackets(unsigned char* bufferAddrP, unsigned int bufferLenP);

private:
    std::queue<std::string> tsPackets;
    std::mutex queueMutex;

private:
    std::vector<std::string> prepareStreamCmd(const m3u_stream& stream);
};
