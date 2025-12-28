#include <fstream>

#include "protocolm3us.h"
#include "common.h"
#include "config.h"
#include "log.h"

cIptvProtocolM3US::cIptvProtocolM3US() : isActiveM(false), handler("M3US") {
    debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolM3US::~cIptvProtocolM3US() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Drop open handles
    cIptvProtocolM3US::Close();
    handler.stop();
}

int cIptvProtocolM3US::Read(unsigned char *bufferAddrP, unsigned int bufferLenP) {
    // debug16("%s (, %u)", __PRETTY_FUNCTION__, bufferLenP);
    return handler.popPackets(bufferAddrP, bufferLenP);
}

bool cIptvProtocolM3US::Open() {
    debug1("%s", __PRETTY_FUNCTION__);

    if (!isActiveM) {
        isActiveM = true;

        auto streams = m3u8Handler.parseM3u(url, useYtdlp);
        if (streams.width==0 || streams.height==0) {
            mark404Channel(channelId);

            cString errmsg = cString::sprintf("Unable to load stream with URL %s", url.c_str());
            debug1("%s", *errmsg);

            errmsg = cString::sprintf(tr("Unable to load stream"));
            Skins.Message(mtError, errmsg);

            isActiveM = false;

            return false;
        }

        {
            LOCK_CHANNELS_READ;
            const cChannel *Channel = Channels->GetByNumber(channelId);
            if (Channel) {
                streams.channelName = Channel->Name();
                streams.vpid = Channel->Vpid();
                streams.spid = Channel->Sid();
                streams.tpid = Channel->Tid();
                streams.nid = Channel->Nid();

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

        handler.setChannelId(channelId);
        handler.setHandlerType(handlerType);

        m3u8Handler.printStream(streams);
        handler.streamVideo(streams);
    }

    return true;
}

bool cIptvProtocolM3US::Close() {
    debug1("%s", __PRETTY_FUNCTION__);

    isActiveM = false;
    handler.stop();

    return true;
}

bool
cIptvProtocolM3US::SetSource(SourceParameter parameter) {
    debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, parameter.locationP, parameter.parameterP, parameter.indexP);

    this->useYtdlp = parameter.useYtDlp;

    // url = findUrl(parameter.parameterP, parameter.locationP);
    url = parameter.locationP;

    if (!url.empty()) {
        url = ReplaceAll(url, "%3A", ":");
        url = ReplaceAll(url, "%7C", "|");
    } else {
        return false;
    }

    channelId = parameter.channelNumber;
    handlerType = parameter.handlerType;

    return true;
}

bool cIptvProtocolM3US::SetPid(int pidP, int typeP, bool onP) {
    debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);

    return true;
}

cString cIptvProtocolM3US::GetInformation() {
    debug16("%s", __PRETTY_FUNCTION__);

    return cString::sprintf("%s", url.c_str());
}
