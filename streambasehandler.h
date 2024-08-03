#pragma once

#include <string>
#include <thread>
#include <queue>
#include "process.hpp"
#include "m3u8handler.h"

class StreamBaseHandler {
private:
    TinyProcessLib::Process *streamHandler;

public:
    StreamBaseHandler();
    virtual ~StreamBaseHandler();

    bool streamVideo(const m3u_stream &stream);
    bool streamAudio(const m3u_stream &stream);
    void stop();
    bool isRunning(int &exit_status);

    int popPackets(unsigned char *bufferAddrP, unsigned int bufferLenP);

private:
    std::queue<std::string> tsPackets;
    std::mutex queueMutex;

protected:
    virtual std::vector<std::string> prepareStreamCmdVideo(const m3u_stream &stream) = 0;
    virtual std::vector<std::string> prepareStreamCmdAudio(const m3u_stream &stream) = 0;
};
