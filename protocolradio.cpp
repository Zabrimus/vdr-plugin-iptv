#include "protocolradio.h"
#include "common.h"
#include "config.h"
#include "streambasehandler.h"

#include "log.h"

cIptvProtocolRadio::cIptvProtocolRadio() : isActiveM(false), handler("RADIO") {
    debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolRadio::~cIptvProtocolRadio() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Drop open handles
    cIptvProtocolRadio::Close();
    handler.stop();
}

int cIptvProtocolRadio::Read(unsigned char *bufferAddrP, unsigned int bufferLenP) {
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
                streams.nid  = Channel->Nid();

                int aidx = 0;
                while (true) {
                    int apid = Channel->Apid(aidx);
                    if (apid==0) {
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

bool
cIptvProtocolRadio::SetSource(SourceParameter parameter) {
    debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, parameter.locationP, parameter.parameterP, parameter.indexP);

    url = parameter.locationP;

    url = ReplaceAll(url, "%3A", ":");
    url = ReplaceAll(url, "%7C", "|");

    if (url.empty()) {
        error("URL %s not found", parameter.locationP);
        return false;
    }

    channelId = parameter.channelNumber;
    handlerType = parameter.handlerType;

    handler.setChannelId(channelId);
    handler.setHandlerType(handlerType);

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
