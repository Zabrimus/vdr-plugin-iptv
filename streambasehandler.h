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
    char handlerType;
    std::string type;

public:
    explicit StreamBaseHandler(std::string type);
    virtual ~StreamBaseHandler();

    void setChannelId(int channelId);
    void setHandlerType(char handlerType);

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
    std::vector<std::string> prepareStreamCmdVideoFfmpeg(const m3u_stream &stream);
    std::vector<std::string> prepareStreamCmdAudioFfmpeg(const m3u_stream &stream);

    std::vector<std::string> prepareStreamCmdVideoVlc(const m3u_stream &stream);
    std::vector<std::string> prepareStreamCmdAudioVlc(const m3u_stream &stream);
};
