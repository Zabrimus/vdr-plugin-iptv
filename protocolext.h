/*
 * protocolext.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <arpa/inet.h>
#include "protocolif.h"
#include "socket.h"

class cIptvProtocolExt : public cIptvUdpSocket, public cIptvProtocolIf {
private:
    int pidM;
    cString scriptFileM;
    int scriptParameterM;
    int streamPortM;

private:
    void TerminateScript();
    void ExecuteScript();

public:
    cIptvProtocolExt();
    ~cIptvProtocolExt() override;
    int Read(unsigned char *bufferAddrP, unsigned int bufferLenP) override;
    bool SetSource(SourceParameter parameter) override;
    bool SetPid(int pidP, int typeP, bool onP) override;
    bool Open() override;
    bool Close() override;
    cString GetInformation() override;
};
