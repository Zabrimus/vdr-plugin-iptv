/*
 * protocoludp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <arpa/inet.h>
#include "protocolif.h"
#include "socket.h"

class cIptvProtocolUdp : public cIptvUdpSocket, public cIptvProtocolIf {
private:
    bool isIGMPv3M;
    char *sourceAddrM;
    char *streamAddrM;
    int streamPortM;

public:
    cIptvProtocolUdp();
    ~cIptvProtocolUdp() override;
    int Read(unsigned char *bufferAddrP, unsigned int bufferLenP) override;
    bool SetSource(SourceParameter parameter) override;
    bool SetPid(int pidP, int typeP, bool onP) override;
    bool Open() override;
    bool Close() override;
    cString GetInformation() override;
};

