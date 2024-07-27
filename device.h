/*
 * device.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <vdr/device.h>
#include "common.h"
#include "deviceif.h"
#include "protocoludp.h"
#include "protocolcurl.h"
#include "protocolhttp.h"
#include "protocolfile.h"
#include "protocolext.h"
#include "protocolm3u.h"
#include "protocolradio.h"
#include "protocolstream.h"
#include "streamer.h"
#include "sectionfilter.h"
#include "pidscanner.h"
#include "sidscanner.h"
#include "statistics.h"

class cIptvDevice : public cDevice, public cIptvPidStatistics, public cIptvBufferStatistics, public cIptvDeviceIf {
    // static ones
public:
    static unsigned int deviceCount;
    static bool Initialize(unsigned int DeviceCount);
    static void Shutdown();
    static unsigned int Count();
    static cIptvDevice *GetIptvDevice(int CardIndex);

    // private parts
private:
    unsigned int deviceIndexM;
    int dvrFdM;
    bool isPacketDeliveredM;
    bool isOpenDvrM;
    bool sidScanEnabledM;
    bool pidScanEnabledM;
    cRingBufferLinear *tsBufferM;
    cChannel channelM;
    cIptvProtocolUdp *pUdpProtocolM;
    cIptvProtocolCurl *pCurlProtocolM;
    cIptvProtocolHttp *pHttpProtocolM;
    cIptvProtocolFile *pFileProtocolM;
    cIptvProtocolExt *pExtProtocolM;
    cIptvProtocolM3U *pM3UProtocolM;
    cIptvProtocolRadio *pRadioProtocolM;
    cIptvProtocolStream *pStreamProtocolM;
    cIptvStreamer *pIptvStreamerM;
    cIptvSectionFilterHandler *pIptvSectionM;
    cPidScanner *pPidScannerM;
    cSidScanner *pSidScannerM;
    cMutex mutexM;

    // constructor & destructor
public:
    explicit cIptvDevice(unsigned int deviceIndexP);
    ~cIptvDevice() override;
    cString GetInformation(unsigned int pageP = IPTV_DEVICE_INFO_ALL);

    // copy and assignment constructors
private:
    cIptvDevice(const cIptvDevice &);
    cIptvDevice &operator=(const cIptvDevice &);

    // for statistics and general information
    cString GetGeneralInformation();
    cString GetPidsInformation();
    cString GetFiltersInformation();

    // for channel info
public:
    cString DeviceType() const override;
    cString DeviceName() const override;
    int SignalStrength() const override;
    int SignalQuality() const override;

    // for channel selection
public:
    bool ProvidesSource(int sourceP) const override;
    bool ProvidesTransponder(const cChannel *channelP) const override;
    bool ProvidesChannel(const cChannel *channelP, int priorityP, bool *needsDetachReceiversP) const override;
    bool ProvidesEIT() const override;
    int NumProvidedSystems() const override;
    const cChannel *GetCurrentlyTunedTransponder() const override;
    bool IsTunedToTransponder(const cChannel *channelP) const override;
    bool MaySwitchTransponder(const cChannel *channelP) const override;

protected:
    bool SetChannelDevice(const cChannel *channelP, bool liveViewP) override;

    // for recording
private:
    uchar *GetData(int *availableP = nullptr);
    void SkipData(int countP);

protected:
    bool SetPid(cPidHandle *handleP, int typeP, bool onP) override;
    bool OpenDvr() override;
    void CloseDvr() override;
    bool GetTSPacket(uchar *&dataP) override;

    // for section filtering
public:
    int OpenFilter(u_short pidP, u_char tidP, u_char maskP) override;
    void CloseFilter(int handleP) override;

    // for transponder lock
public:
    bool HasLock(int timeoutMsP) const override;

    // for common interface
public:
    bool HasInternalCam() override;

    // for internal device interface
public:
    void WriteData(u_char *bufferP, int lengthP) override;
    unsigned int CheckData() override;
};
