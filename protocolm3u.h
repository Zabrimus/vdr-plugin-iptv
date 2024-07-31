#pragma once

#include <string>
#include <vdr/tools.h>
#include <vdr/channels.h>
#include "protocolif.h"
#include "m3u8handler.h"
#include "ffmpeghandler.h"

class cIptvProtocolM3U : public cIptvProtocolIf {
private:
    int channelId;
    std::string url;
    bool isActiveM;
    int useYtdlp;
    M3u8Handler m3u8Handler;
    FFmpegHandler handler;

public:
    cIptvProtocolM3U();
    ~cIptvProtocolM3U() override;
    int Read(unsigned char *bufferAddrP, unsigned int bufferLenP) override;
    bool SetSource(const char *locationP, int parameterP, int indexP, int channelNumber, int useYtDlp) override;
    bool SetPid(int pidP, int typeP, bool onP) override;
    bool Open() override;
    bool Close() override;
    cString GetInformation() override;
};
