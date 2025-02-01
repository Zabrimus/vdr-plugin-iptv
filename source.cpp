/*
 * source.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <cctype>

#include "common.h"
#include "log.h"
#include "source.h"

// --- cIptvTransponderParameters --------------------------------------------

cIptvTransponderParameters::cIptvTransponderParameters(const char *parametersP)
    : sidScanM(0),
      pidScanM(0),
      protocolM(eProtocolUDP),
      parameterM(0),
      useYtdlp(0),
      handlerType('F') {
    debug1("%s (%s)", __PRETTY_FUNCTION__, parametersP);

    memset(&addressM, 0, sizeof(addressM));
    Parse(parametersP);
}

std::string cIptvTransponderParameters::ToString(char typeP) const {
    debug1("%s (%c)", __PRETTY_FUNCTION__, typeP);

    std::string tostr;

    tostr.append("S=").append(std::to_string(sidScanM));
    tostr.append("|P=").append(std::to_string(pidScanM));
    tostr.append("|F=").append(ProtocolToStr(protocolM));
    tostr.append("|U=").append(addressM);
    tostr.append("|A=").append(std::to_string(parameterM));

    if (useYtdlp > 0) {
        tostr.append("|Y=").append(std::to_string(useYtdlp));
    }

    tostr.append("|H=").append(std::string(1 , handlerType));

    if (!xmltvId.empty()) {
        tostr.append("|X=").append(xmltvId);
    }

    return { tostr };
}

int cIptvTransponderParameters::StrToProtocol(const char *prot) {
    int protocolM = -1;

    if (strstr(prot, "UDP")) {
        protocolM = eProtocolUDP;
    } else if (strstr(prot, "CURL")) {
        protocolM = eProtocolCURL;
    } else if (strstr(prot, "HTTP")) {
        protocolM = eProtocolHTTP;
    } else if (strstr(prot, "FILE")) {
        protocolM = eProtocolFILE;
    } else if (strstr(prot, "EXTT")) {
        protocolM = eProtocolEXTT;
    } else if (strstr(prot, "EXT")) {
        protocolM = eProtocolEXT;
    } else if (strstr(prot, "M3US")) {
        protocolM = eProtocolM3US;
    } else if (strstr(prot, "M3U")) {
        protocolM = eProtocolM3U;
    } else if (strstr(prot, "RADIO")) {
        protocolM = eProtocolRadio;
    } else if (strstr(prot, "STREAM")) {
        protocolM = eProtocolStream;
    } else if (strstr(prot, "YT")) {
        protocolM = eProtocolYT;
    }

    return protocolM;
}

std::string cIptvTransponderParameters::ProtocolToStr(int prot) {
    std::string protocolstr;

    switch (prot) {
    case eProtocolEXT:
        protocolstr = "EXT";
        break;

    case eProtocolEXTT:
        protocolstr = "EXTT";
        break;

    case eProtocolCURL:
        protocolstr = "CURL";
        break;

    case eProtocolHTTP:
        protocolstr = "HTTP";
        break;

    case eProtocolFILE:
        protocolstr = "FILE";
        break;

    case eProtocolM3U:
        protocolstr = "M3U";
        break;

    case eProtocolM3US:
        protocolstr = "M3US";
        break;

    case eProtocolRadio:
        protocolstr = "RADIO";
        break;

    case eProtocolStream:
        protocolstr = "STREAM";
        break;

    case eProtocolYT:
        protocolstr = "YT";
        break;

    default:
    case eProtocolUDP:
        protocolstr = "UDP";
        break;

    }

    return protocolstr;
}

bool cIptvTransponderParameters::Parse(const char *strP) {
    debug1("%s (%s)", __PRETTY_FUNCTION__, strP);

    bool result = false;
    int tmpProto;

    if (strP && *strP) {
        const char *delim = "|";
        char *str = strdup(strP);
        char *p = str;
        char *saveptr = nullptr;
        char *token = nullptr;
        bool found_s = false;
        bool found_p = false;
        bool found_f = false;
        bool found_u = false;
        bool found_a = false;

        while ((token = strtok_r(str, delim, &saveptr))!=nullptr) {
            char *data = token;
            ++data;
            if (data && (*data=='=')) {
                ++data;

                switch (toupper(*token)) {
                case 'S':
                    sidScanM = (int) strtol(data, (char **) nullptr, 10);
                    found_s = true;
                    break;

                case 'P':
                    pidScanM = (int) strtol(data, (char **) nullptr, 10);
                    found_p = true;
                    break;

                case 'F':
                    tmpProto = StrToProtocol(data);
                    if (tmpProto != -1) {
                        protocolM = tmpProto;
                        found_f = true;
                    }
                    break;

                case 'U':
                    strn0cpy(addressM, data, sizeof(addressM));
                    found_u = true;
                    break;

                case 'A':
                    parameterM = (int) strtol(data, (char **) nullptr, 10);
                    found_a = true;
                    break;

                case 'Y':
                    useYtdlp = (int) strtol(data, (char **) nullptr, 10);
                    break;

                case 'H':
                    if (data[0] == 'F' || data[0] == 'V' || data[0] == 'E') {
                        handlerType = data[0];
                    }
                    break;

                case 'X':
                    xmltvId = std::string(data);
                    break;

                default:
                    break;
                }
            }

            str = nullptr;
        }

        // Y is optional and therefore not recognized
        if (found_s && found_p && found_f && found_u && found_a) {
            result = true;
        } else {
            error("Invalid channel parameters: %s", p);
        }

        free(p);
    }

    return (result);
}

// --- cIptvSourceParam ------------------------------------------------------

const char *cIptvSourceParam::allowedProtocolCharsS = " abcdefghijklmnopqrstuvwxyz0123456789-.,#~\\^$[]()*+?{}/%@&=";

cIptvSourceParam::cIptvSourceParam(char sourceP, const char *descriptionP)
    : cSourceParam(sourceP, descriptionP),
      paramM(0),
      ridM(0),
      dataM(),
      itpM() {
    debug1("%s (%c, %s)", __PRETTY_FUNCTION__, sourceP, descriptionP);

    protocolsM[cIptvTransponderParameters::eProtocolUDP] = tr("UDP");
    protocolsM[cIptvTransponderParameters::eProtocolCURL] = tr("CURL");
    protocolsM[cIptvTransponderParameters::eProtocolHTTP] = tr("HTTP");
    protocolsM[cIptvTransponderParameters::eProtocolFILE] = tr("FILE");
    protocolsM[cIptvTransponderParameters::eProtocolEXT] = tr("EXT");
    protocolsM[cIptvTransponderParameters::eProtocolEXTT] = tr("EXTT");
    protocolsM[cIptvTransponderParameters::eProtocolM3U] = tr("M3U");
    protocolsM[cIptvTransponderParameters::eProtocolM3US] = tr("M3US");
    protocolsM[cIptvTransponderParameters::eProtocolRadio] = tr("RADIO");
    protocolsM[cIptvTransponderParameters::eProtocolStream] = tr("STREAM");
    protocolsM[cIptvTransponderParameters::eProtocolYT] = tr("YT");
}

void cIptvSourceParam::SetData(cChannel *channelP) {
    debug1("%s (%s)", __PRETTY_FUNCTION__, channelP->Parameters());

    dataM = *channelP;
    ridM = dataM.Rid();
    itpM.Parse(dataM.Parameters());
    paramM = 0;
}

void cIptvSourceParam::GetData(cChannel *channelP) {
    debug1("%s (%s)", __PRETTY_FUNCTION__, channelP->Parameters());

    channelP->SetTransponderData(channelP->Source(),
                                 channelP->Frequency(),
                                 dataM.Srate(),
                                 itpM.ToString(Source()).c_str(),
                                 true);
    channelP->SetId(nullptr, channelP->Nid(), channelP->Tid(), channelP->Sid(), ridM);
}

cOsdItem *cIptvSourceParam::GetOsdItem() {
    debug1("%s", __PRETTY_FUNCTION__);

    switch (paramM++) {
    case 0: return new cMenuEditIntItem(tr("Rid"), &ridM, 0);
    case 1: return new cMenuEditBoolItem(tr("Scan section ids"), &itpM.sidScanM);
    case 2: return new cMenuEditBoolItem(tr("Scan pids"), &itpM.pidScanM);
    case 3: return new cMenuEditStraItem(tr("Protocol"), &itpM.protocolM, ELEMENTS(protocolsM), protocolsM);
    case 4: return new cMenuEditStrItem(tr("Address"), itpM.addressM, sizeof(itpM.addressM), allowedProtocolCharsS);
    case 5: return new cMenuEditIntItem(tr("Parameter"), &itpM.parameterM, 0, 0xFFFF);
    default:
        return nullptr;
    }
}