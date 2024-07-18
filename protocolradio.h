#pragma once

#include <string>
#include <vdr/tools.h>
#include <vdr/channels.h>
#include "protocolif.h"
#include "m3u8handler.h"
#include "ffmpeghandler.h"

class cIptvProtocolRadio: public cIptvProtocolIf{
private:
    int channelId;
    std::string url;
    bool isActiveM;
    FFmpegHandler handler;

public:
    cIptvProtocolRadio();
    virtual ~cIptvProtocolRadio();
    int Read(unsigned char* bufferAddrP, unsigned int bufferLenP);
    bool SetSource(const char *locationP, const int parameterP, const int indexP, int channelNumber);
    bool SetPid(int pidP, int typeP, bool onP);
    bool Open(void);
    bool Close(void);
    cString GetInformation(void);
};
