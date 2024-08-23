#pragma once

#include <string>
#include <vdr/tools.h>
#include <vdr/channels.h>
#include "protocolif.h"
#include "m3u8handler.h"
#include "streambasehandler.h"

class cIptvProtocolM3U : public cIptvProtocolIf {
private:
    int channelId;
    std::string url;
    bool isActiveM;
    int useYtdlp;
    M3u8Handler m3u8Handler;
    StreamBaseHandler handler;
    char handlerType;

public:
    cIptvProtocolM3U();
    ~cIptvProtocolM3U() override;
    int Read(unsigned char *bufferAddrP, unsigned int bufferLenP) override;
    bool SetSource(SourceParameter parameter) override;
    bool SetPid(int pidP, int typeP, bool onP) override;
    bool Open() override;
    bool Close() override;
    cString GetInformation() override;
};
