#pragma once

#include <string>
#include <thread>
#include <queue>
#include "process.hpp"
#include "m3u8handler.h"

class StreamBaseHandler {
private:
    TinyProcessLib::Process *streamHandler;
    int channelId;

public:
    explicit StreamBaseHandler(int channelId);
    virtual ~StreamBaseHandler();

    bool streamVideo(const m3u_stream &stream);
    bool streamAudio(const m3u_stream &stream);
    void stop();

    int popPackets(unsigned char *bufferAddrP, unsigned int bufferLenP);

private:
    std::deque<std::string> tsPackets;
    std::mutex queueMutex;

    std::thread streamThread;
    void streamVideoInternal(const m3u_stream &stream);
    void streamAudioInternal(const m3u_stream &stream);

    void checkErrorOut(const std::string &msg);

protected:
    virtual std::vector<std::string> prepareStreamCmdVideo(const m3u_stream &stream) = 0;
    virtual std::vector<std::string> prepareStreamCmdAudio(const m3u_stream &stream) = 0;
};
