/*
 * protocolhttp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <arpa/inet.h>
#include "protocolif.h"
#include "socket.h"

class cIptvProtocolHttp : public cIptvTcpSocket, public cIptvProtocolIf {
private:
    char *streamAddrM;
    char *streamPathM;
    int streamPortM;

private:
    bool Connect();
    bool Disconnect();
    bool GetHeaderLine(char *destP, unsigned int destLenP, unsigned int &recvLenP);
    bool ProcessHeaders();

public:
    cIptvProtocolHttp();
    ~cIptvProtocolHttp() override;
    int Read(unsigned char *bufferAddrP, unsigned int bufferLenP) override;
    bool SetSource(const char *locationP, int parameterP, int indexP, int channelNumber, int useYtDlp) override;
    bool SetPid(int pidP, int typeP, bool onP) override;
    bool Open() override;
    bool Close() override;
    cString GetInformation() override;
};

