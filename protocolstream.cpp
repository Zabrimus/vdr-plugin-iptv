#include "protocolstream.h"
#include "common.h"
#include "config.h"
#include "ffmpeghandler.h"
#include "vlchandler.h"
#include "log.h"

cIptvProtocolStream::cIptvProtocolStream() : channelId(0), isActiveM(false), handler(nullptr) {
    debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolStream::~cIptvProtocolStream() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Drop open handles
    cIptvProtocolStream::Close();

    delete handler;
    handler = nullptr;
}

int cIptvProtocolStream::Read(unsigned char *bufferAddrP, unsigned int bufferLenP) {
    // debug16("%s (, %u)", __PRETTY_FUNCTION__, bufferLenP);

    return handler->popPackets(bufferAddrP, bufferLenP);
}

bool cIptvProtocolStream::Open() {
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

        handler->streamVideo(streams);
    }
    return true;
}

bool cIptvProtocolStream::Close() {
    debug1("%s", __PRETTY_FUNCTION__);

    isActiveM = false;

    if (handler != nullptr) {
        handler->stop();
    }

    return true;
}

bool cIptvProtocolStream::SetSource(SourceParameter parameter) {
    debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, parameter.locationP, parameter.parameterP, parameter.indexP);

    url = parameter.locationP;

    url = ReplaceAll(url, "%3A", ":");
    url = ReplaceAll(url, "%7C", "|");

    if (url.empty()) {
        error("URL %s not found", parameter.locationP);
        return false;
    }

    channelId = parameter.channelNumber;

    if (handler != nullptr) {
        handler->stop();
    }

    delete handler;
    handler = nullptr;

    if (parameter.handlerType == 'F') {
        handler = new FFmpegHandler(channelId);
    } else if (parameter.handlerType == 'V') {
        handler = new VlcHandler(channelId);
    }

    return true;
}

bool cIptvProtocolStream::SetPid(int pidP, int typeP, bool onP) {
    debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);

    return true;
}

cString cIptvProtocolStream::GetInformation() {
    debug16("%s", __PRETTY_FUNCTION__);

    return cString::sprintf("%s", url.c_str());
}
