#pragma once

#include <string>
#include <thread>
#include <queue>
#include "process.hpp"
#include "m3u8handler.h"
#include "streambasehandler.h"

class VlcHandler : public StreamBaseHandler {
public:
    VlcHandler();
    ~VlcHandler() override;

protected:
    std::vector<std::string> prepareStreamCmdVideo(const m3u_stream &stream) override;
    std::vector<std::string> prepareStreamCmdAudio(const m3u_stream &stream) override;
};
