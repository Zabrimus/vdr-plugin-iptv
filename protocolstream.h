#pragma once

#include <string>
#include <vdr/tools.h>
#include <vdr/channels.h>
#include "protocolif.h"
#include "m3u8handler.h"
#include "ffmpeghandler.h"

class cIptvProtocolStream : public cIptvProtocolIf {
private:
    int channelId;
    std::string url;
    bool isActiveM;
    FFmpegHandler handler;

public:
    cIptvProtocolStream();
    ~cIptvProtocolStream() override;
    int Read(unsigned char *bufferAddrP, unsigned int bufferLenP) override;
    bool SetSource(const char *locationP, int parameterP, int indexP, int channelNumber, int useYtDlp) override;
    bool SetPid(int pidP, int typeP, bool onP) override;
    bool Open() override;
    bool Close() override;
    cString GetInformation() override;
};
