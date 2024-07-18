#include <fstream>

#include "protocolradio.h"
#include "common.h"
#include "config.h"

#include "log.h"

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

cIptvProtocolRadio::cIptvProtocolRadio() : isActiveM(false) {
    debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolRadio::~cIptvProtocolRadio() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Drop open handles
    cIptvProtocolRadio::Close();
}

int cIptvProtocolRadio::Read(unsigned char* bufferAddrP, unsigned int bufferLenP) {
    // debug16("%s (, %u)", __PRETTY_FUNCTION__, bufferLenP);
    return handler.popPackets(bufferAddrP, bufferLenP);
}

bool cIptvProtocolRadio::Open() {
    debug1("%s", __PRETTY_FUNCTION__);

    if (!isActiveM) {
        isActiveM = true;

        m3u_stream streams;

        {
            LOCK_CHANNELS_READ;
            const cChannel *Channel = Channels->GetByNumber(channelId);
            if (Channel) {
                streams.url = url;
                streams.channelName = Channel->Name();
                streams.vpid = 0;
                streams.spid = Channel->Sid();
                streams.tpid = Channel->Tid();

                int aidx = 0;
                while (true) {
                    int apid = Channel->Apid(aidx);
                    if (apid == 0) {
                        break;
                    }
                    streams.apids.push_back(apid);
                    aidx++;
                }
            }
        }

        handler.streamAudio(streams);
    }
    return true;
}

bool cIptvProtocolRadio::Close() {
    debug1("%s", __PRETTY_FUNCTION__);

    isActiveM = false;
    handler.stop();
    return true;
}

bool cIptvProtocolRadio::SetSource(const char *locationP, const int parameterP, const int indexP, int channelNumber) {
    debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, locationP, parameterP, indexP);

    url = locationP;

    url = ReplaceAll(url, "%3A", ":");
    url = ReplaceAll(url, "%7C", "|");

    if (url.empty()) {
        error("URL %s not found", locationP);
    }

    channelId = channelNumber;
    return true;
}

bool cIptvProtocolRadio::SetPid(int pidP, int typeP, bool onP) {
    debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);
    return true;
}

cString cIptvProtocolRadio::GetInformation() {
    debug16("%s", __PRETTY_FUNCTION__);
    return cString::sprintf("%s", url.c_str());
}
