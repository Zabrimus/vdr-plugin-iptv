#include <fstream>

#include "protocolm3u.h"
#include "common.h"
#include "config.h"
#include "streambasehandler.h"
#include "log.h"

cIptvProtocolM3U::cIptvProtocolM3U() : isActiveM(false), handler("M3U") {
    debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolM3U::~cIptvProtocolM3U() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Drop open handles
    cIptvProtocolM3U::Close();
    handler.stop();
}

int cIptvProtocolM3U::Read(unsigned char *bufferAddrP, unsigned int bufferLenP) {
    // debug16("%s (, %u)", __PRETTY_FUNCTION__, bufferLenP);
    return handler.popPackets(bufferAddrP, bufferLenP);
}

bool cIptvProtocolM3U::Open() {
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

bool cIptvProtocolM3U::Close() {
    debug1("%s", __PRETTY_FUNCTION__);

    isActiveM = false;
    handler.stop();

    return true;
}

bool
cIptvProtocolM3U::SetSource(SourceParameter parameter) {
    debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, parameter.locationP, parameter.parameterP, parameter.indexP);

    this->useYtdlp = parameter.useYtDlp;

    url = findUrl(parameter.parameterP, parameter.locationP);

    if (url.empty()) {
        return false;
    }

    channelId = parameter.channelNumber;
    handlerType = parameter.handlerType;

    /*
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
    */
    return true;
}

std::string cIptvProtocolM3U::findUrl(int parameterP, const char* locationP) {
    std::string m3uUrl;

    struct stat stbuf;
    cString configFileM = cString::sprintf("%s/%s", IptvConfig.GetM3uCfgPath(), locationP);
    if ((stat(*configFileM, &stbuf)!=0) || (strstr(*configFileM, "..") != nullptr)) {
        error("Non-existent or relative configuration file '%s'", *configFileM);

        return "";
    }

    std::ifstream file(configFileM);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            unsigned long idx = line.find(':');
            if (idx > 0) {
                std::string nr = line.substr(0, idx);
                if (nr==std::to_string(parameterP)) {
                    m3uUrl = line.substr(idx + 1);
                    if (m3uUrl.empty()) {
                        error("URL with index %d in file %s not found", parameterP, *configFileM);

                        file.close();
                        return "";
                    }

                    debug1("Found URL %s", m3uUrl.c_str());
                    break;
                }
            }
        }

        file.close();
    }

    if (m3uUrl.empty()) {
        error("URL with index %d in file %s not found", parameterP, *configFileM);
        return "";
    }

    return m3uUrl;
}

bool cIptvProtocolM3U::SetPid(int pidP, int typeP, bool onP) {
    debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);

    return true;
}

cString cIptvProtocolM3U::GetInformation() {
    debug16("%s", __PRETTY_FUNCTION__);

    return cString::sprintf("%s", url.c_str());
}
