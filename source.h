/*
 * source.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <vdr/menuitems.h>
#include <vdr/sourceparams.h>
#include "common.h"

class cIptvTransponderParameters {
    friend class cIptvSourceParam;

private:
    int sidScanM;
    int pidScanM;
    int protocolM;
    char addressM[NAME_MAX + 1];
    int parameterM;
    int useYtdlp;
    char handlerType;
    std::string xmltvId;

public:
    enum {
        eProtocolUDP,
        eProtocolCURL,
        eProtocolHTTP,
        eProtocolFILE,
        eProtocolEXT,
        eProtocolM3U,
        eProtocolRadio,
        eProtocolStream,
        eProtocolCount
    };
    explicit cIptvTransponderParameters(const char *parametersP = nullptr);
    int SidScan() const { return sidScanM; };
    int PidScan() const { return pidScanM; };
    int Protocol() const { return protocolM; };
    const char *Address() const { return addressM; };
    int Parameter() const { return parameterM; };
    int UseYtdlp() const { return useYtdlp; };
    char HandlerType() const { return handlerType; };
    std::string XmlTVId() const { return xmltvId; };

    void SetSidScan(int sidScanP) { sidScanM = sidScanP; };
    void SetPidScan(int pidScanP) { pidScanM = pidScanP; };
    void SetProtocol(int protocolP) { protocolM = protocolP; };
    void SetAddress(const char *addressP) { strncpy(addressM, addressP, sizeof(addressM)); };
    void SetParameter(int parameterP) { parameterM = parameterP; };
    void SetYtdlp(int value) { useYtdlp = value; };
    void SetXmlTvId(std::string id) { xmltvId = id; };
    void SetHandlerType(char type) { handlerType = type; };

    cString ToString(char typeP) const;
    bool Parse(const char *strP);
};

class cIptvSourceParam : public cSourceParam {
private:
    int paramM;
    int ridM;
    cChannel dataM;
    cIptvTransponderParameters itpM;
    const char *protocolsM[cIptvTransponderParameters::eProtocolCount];

private:
    static const char *allowedProtocolCharsS;

public:
    cIptvSourceParam(char sourceP, const char *descriptionP);
    void SetData(cChannel *channelP) override;
    void GetData(cChannel *channelP) override;
    cOsdItem *GetOsdItem() override;
};
