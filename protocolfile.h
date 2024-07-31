/*
 * protocolfile.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <arpa/inet.h>
#include "protocolif.h"

class cIptvProtocolFile : public cIptvProtocolIf {
private:
    char *fileLocationM;
    int fileDelayM;
    FILE *fileStreamM;
    bool isActiveM;

private:
    bool OpenFile();
    void CloseFile();

public:
    cIptvProtocolFile();
    ~cIptvProtocolFile() override;
    int Read(unsigned char *bufferAddrP, unsigned int bufferLenP) override;
    bool SetSource(const char *locationP, int parameterP, int indexP, int channelNumber, int useYtDlp) override;
    bool SetPid(int pidP, int typeP, bool onP) override;
    bool Open() override;
    bool Close() override;
    cString GetInformation() override;
};

