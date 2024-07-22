/*
 * streamer.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <mutex>
#include <arpa/inet.h>

#include <vdr/thread.h>

#include "deviceif.h"
#include "protocolif.h"
#include "statistics.h"

class cIptvStreamer : public cThread, public cIptvStreamerStatistics {
private:
    cCondWait sleepM;
    cIptvDeviceIf *deviceM;
    unsigned char *packetBufferM;
    unsigned int packetBufferLenM;
    cIptvProtocolIf *protocolM;

    std::mutex streamerMutex;

protected:
    void Action() override;

public:
    cIptvStreamer(cIptvDeviceIf &deviceP, unsigned int packetLenP);
    ~cIptvStreamer() override;

    bool SetSource(const char *locationP, const int parameterP, const int indexP, cIptvProtocolIf *protocolP, int channelNumber);
    bool SetPid(int pidP, int typeP, bool onP);
    bool Open();
    bool Close();
    cString GetInformation();
};
