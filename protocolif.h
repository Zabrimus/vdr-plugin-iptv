/*
 * protocolif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

typedef struct {
    const char *locationP;
    int parameterP;
    unsigned int indexP;
    int channelNumber;
    int useYtDlp;
    char handlerType;
} SourceParameter;

class cIptvProtocolIf {
public:
    cIptvProtocolIf() = default;
    virtual ~cIptvProtocolIf() = default;
    virtual int Read(unsigned char *bufferAddrP, unsigned int bufferLenP) = 0;
    virtual bool SetSource(SourceParameter parameter) = 0;
    virtual bool SetPid(int pidP, int typeP, bool onP) = 0;
    virtual bool Open() = 0;
    virtual bool Close() = 0;
    virtual cString GetInformation() = 0;

private:
    cIptvProtocolIf(const cIptvProtocolIf &);
    cIptvProtocolIf &operator=(const cIptvProtocolIf &);
};
