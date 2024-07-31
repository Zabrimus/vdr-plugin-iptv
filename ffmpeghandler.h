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

    bool streamVideo(const m3u_stream &stream);
    bool streamAudio(const m3u_stream &stream);
    void stop();
    bool isRunning(int &exit_status);

    int popPackets(unsigned char *bufferAddrP, unsigned int bufferLenP);

private:
    std::queue<std::string> tsPackets;
    std::mutex queueMutex;

private:
    static std::vector<std::string> prepareStreamCmdVideo(const m3u_stream &stream);
    static std::vector<std::string> prepareStreamCmdAudio(const m3u_stream &stream);
};
