/*
 * protocolif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

class cIptvProtocolIf {
public:
    cIptvProtocolIf() = default;
    virtual ~cIptvProtocolIf() = default;
    virtual int Read(unsigned char *bufferAddrP, unsigned int bufferLenP) = 0;
    virtual bool SetSource(const char *locationP, int parameterP, int indexP, int channelNumber, int useYtDlp) = 0;
    virtual bool SetPid(int pidP, int typeP, bool onP) = 0;
    virtual bool Open() = 0;
    virtual bool Close() = 0;
    virtual cString GetInformation() = 0;

private:
    cIptvProtocolIf(const cIptvProtocolIf &);
    cIptvProtocolIf &operator=(const cIptvProtocolIf &);
};
