#include <fstream>

#include "protocolm3u.h"
#include "common.h"
#include "config.h"

#include "log.h"

cIptvProtocolM3U::cIptvProtocolM3U() : isActiveM(false) {
    debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolM3U::~cIptvProtocolM3U() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Drop open handles
    cIptvProtocolM3U::Close();
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
            debug1("Unable to read URL '%s'\n", url.c_str());

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
cIptvProtocolM3U::SetSource(const char *locationP,
                            const int parameterP,
                            const int indexP,
                            int channelNumber,
                            int useYtDlp) {
    debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, locationP, parameterP, indexP);

    this->useYtdlp = useYtDlp;

    struct stat stbuf;
    cString configFileM = cString::sprintf("%s/%s", IptvConfig.GetResourceDirectory(), locationP);
    if ((stat(*configFileM, &stbuf)!=0) || (strstr(*configFileM, "..") != nullptr)) {
        error("Non-existent or relative configuration file '%s'", *configFileM);

        return false;
    }

    std::ifstream file(configFileM);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            unsigned long idx = line.find(':');
            if (idx > 0) {
                std::string nr = line.substr(0, idx);
                if (nr==std::to_string(parameterP)) {
                    url = line.substr(idx + 1);
                    if (url.empty()) {
                        error("URL with index %d in file %s not found", parameterP, *configFileM);

                        file.close();
                        return false;
                    }

                    debug1("Found URL %s", url.c_str());
                    break;
                }
            }
        }

        file.close();
    }

    if (url.empty()) {
        error("URL with index %d in file %s not found", parameterP, *configFileM);
    }

    channelId = channelNumber;
    return true;
}

bool cIptvProtocolM3U::SetPid(int pidP, int typeP, bool onP) {
    debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);

    return true;
}

cString cIptvProtocolM3U::GetInformation() {
    debug16("%s", __PRETTY_FUNCTION__);

    return cString::sprintf("%s", url.c_str());
}
