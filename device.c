/*
 * device.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: device.c,v 1.64 2007/10/09 17:59:12 ajhseppa Exp $
 */

#include "config.h"
#include "device.h"

#define IPTV_MAX_DEVICES 8

cIptvDevice * IptvDevices[IPTV_MAX_DEVICES] = { NULL };

unsigned int cIptvDevice::deviceCount = 0;

cIptvDevice::cIptvDevice(unsigned int Index)
: deviceIndex(Index),
  isPacketDelivered(false),
  isOpenDvr(false),
  mutex()
{
  debug("cIptvDevice::cIptvDevice(%d)\n", deviceIndex);
  tsBuffer = new cRingBufferLinear(MEGABYTE(IptvConfig.GetTsBufferSize()),
                                   (TS_SIZE * IptvConfig.GetReadBufferTsCount()),
                                   false, "IPTV");
  tsBuffer->SetTimeouts(100, 100);
  ResetBuffering();
  pUdpProtocol = new cIptvProtocolUdp();
  pHttpProtocol = new cIptvProtocolHttp();
  pFileProtocol = new cIptvProtocolFile();
  pIptvStreamer = new cIptvStreamer(tsBuffer, &mutex);
  // Initialize filter pointers
  memset(&secfilters, '\0', sizeof(secfilters));
  // Start section handler for iptv device
  StartSectionHandler();
  // Sid scanner must be created after the section handler
  pSidScanner = new cSidScanner;
  if (pSidScanner)
     AttachFilter(pSidScanner);
}

cIptvDevice::~cIptvDevice()
{
  debug("cIptvDevice::~cIptvDevice(%d)\n", deviceIndex);
  DELETENULL(pIptvStreamer);
  DELETENULL(pUdpProtocol);
  DELETENULL(pHttpProtocol);
  DELETENULL(pFileProtocol);
  DELETENULL(tsBuffer);
  // Detach and destroy sid filter
  if (pSidScanner) {
     Detach(pSidScanner);
     DELETENULL(pSidScanner);
     }
  // Destroy all filters
  for (int i = 0; i < eMaxSecFilterCount; ++i)
      DeleteFilter(i);
}

bool cIptvDevice::Initialize(unsigned int DeviceCount)
{
  debug("cIptvDevice::Initialize(): DeviceCount=%d\n", DeviceCount);
  if (DeviceCount > IPTV_MAX_DEVICES)
     DeviceCount = IPTV_MAX_DEVICES;
  for (unsigned int i = 0; i < DeviceCount; ++i)
      IptvDevices[i] = new cIptvDevice(i);
  for (unsigned int i = DeviceCount; i < IPTV_MAX_DEVICES; ++i)
      IptvDevices[i] = NULL;
  return true;
}

unsigned int cIptvDevice::Count(void)
{
  unsigned int count = 0;
  debug("cIptvDevice::Count()\n");
  for (unsigned int i = 0; i < IPTV_MAX_DEVICES; ++i) {
      if (IptvDevices[i] != NULL)
         count++;
      }
  return count;
}

cIptvDevice *cIptvDevice::GetIptvDevice(int CardIndex)
{
  //debug("cIptvDevice::GetIptvDevice(%d)\n", CardIndex);
  for (unsigned int i = 0; i < IPTV_MAX_DEVICES; ++i) {
      if ((IptvDevices[i] != NULL) && (IptvDevices[i]->CardIndex() == CardIndex)) {
         //debug("cIptvDevice::GetIptvDevice(%d): FOUND!\n", CardIndex);
         return IptvDevices[i];
         }
      }
  return NULL;
}

cString cIptvDevice::GetGeneralInformation(void)
{
  //debug("cIptvDevice::GetGeneralInformation(%d)\n", deviceIndex);
  return cString::sprintf("IPTV device #%d (CardIndex: %d)\n%s\n%s\nTS Buffer: %s\nStream Buffer: %s\n",
                          deviceIndex, CardIndex(), pIptvStreamer ?
                          *pIptvStreamer->GetInformation() : "",
                          pIptvStreamer ? *pIptvStreamer->cIptvStreamerStatistics::GetStatistic() : "",
			  *cIptvBufferStatistics::GetStatistic(),
                          pIptvStreamer ? *pIptvStreamer->cIptvBufferStatistics::GetStatistic() : "");
}

cString cIptvDevice::GetPidsInformation(void)
{
  //debug("cIptvDevice::GetPidsInformation(%d)\n", deviceIndex);
  cString info("Most active pids:\n");
  info = cString::sprintf("%s%s", *info, *cIptvDeviceStatistics::GetStatistic());
  return info;
}

cString cIptvDevice::GetFiltersInformation(void)
{
  //debug("cIptvDevice::GetFiltersInformation(%d)\n", deviceIndex);
  unsigned int count = 0;
  cString info("Active section filters:\n");
  // loop through active section filters
  for (unsigned int i = 0; i < eMaxSecFilterCount; ++i) {
      if (secfilters[i]) {
         info = cString::sprintf("%sFilter %d: %s Pid=%d\n", *info, i,
                                 *secfilters[i]->GetStatistic(), secfilters[i]->GetPid());
         if (++count > IPTV_STATS_ACTIVE_FILTERS_COUNT)
            break;
         }
      }
  return info;
}

cString cIptvDevice::GetInformation(unsigned int Page)
{
  // generate information string
  cString info;
  switch (Page) {
    case IPTV_DEVICE_INFO_GENERAL:
         info = GetGeneralInformation();
         break;
    case IPTV_DEVICE_INFO_PIDS:
         info = GetPidsInformation();
         break;
    case IPTV_DEVICE_INFO_FILTERS:
         info = GetFiltersInformation();
         break;
    default:
         info = cString::sprintf("%s%s%s",
                                 *GetGeneralInformation(),
                                 *GetPidsInformation(),
                                 *GetFiltersInformation());
         break;
    }
  return info;
}

cString cIptvDevice::GetChannelSettings(const char *Param, int *IpPort, cIptvProtocolIf* *Protocol)
{
  debug("cIptvDevice::GetChannelSettings(%d)\n", deviceIndex);
  char *loc = NULL;
  if (sscanf(Param, "IPTV|UDP|%a[^|]|%u", &loc, IpPort) == 2) {
     cString addr(loc, true);
     *Protocol = pUdpProtocol;
     return addr;
     }
  else if (sscanf(Param, "IPTV|HTTP|%a[^|]|%u", &loc, IpPort) == 2) {
     cString addr(loc, true);
     *Protocol = pHttpProtocol;
     return addr;
     }
  else if (sscanf(Param, "IPTV|FILE|%a[^|]|%u", &loc, IpPort) == 2) {
     cString addr(loc, true);
     *Protocol = pFileProtocol;
     return addr;
     }
  return NULL;
}

bool cIptvDevice::ProvidesIptv(const char *Param) const
{
  debug("cIptvDevice::ProvidesIptv(%d)\n", deviceIndex);
  return (strncmp(Param, "IPTV", 4) == 0);
}

bool cIptvDevice::ProvidesSource(int Source) const
{
  debug("cIptvDevice::ProvidesSource(%d)\n", deviceIndex);
  return (cSource::IsPlug(Source));
}

bool cIptvDevice::ProvidesTransponder(const cChannel *Channel) const
{
  debug("cIptvDevice::ProvidesTransponder(%d)\n", deviceIndex);
  return (ProvidesSource(Channel->Source()) && ProvidesIptv(Channel->PluginParam()));
}

bool cIptvDevice::ProvidesChannel(const cChannel *Channel, int Priority, bool *NeedsDetachReceivers) const
{
  bool result = false;
  bool needsDetachReceivers = false;

  debug("cIptvDevice::ProvidesChannel(%d)\n", deviceIndex);
  if (ProvidesTransponder(Channel))
     result = true;
  if (NeedsDetachReceivers)
     *NeedsDetachReceivers = needsDetachReceivers;
  return result;
}

bool cIptvDevice::SetChannelDevice(const cChannel *Channel, bool LiveView)
{
  int port;
  cString addr;
  cIptvProtocolIf *protocol;

  debug("cIptvDevice::SetChannelDevice(%d)\n", deviceIndex);
  addr = GetChannelSettings(Channel->PluginParam(), &port, &protocol);
  if (isempty(addr)) {
     error("ERROR: Unrecognized IPTV channel settings: %s", Channel->PluginParam());
     return false;
     }
  pIptvStreamer->Set(addr, port, protocol);
  if (pSidScanner && IptvConfig.GetSectionFiltering() && IptvConfig.GetSidScanning())
     pSidScanner->SetChannel(Channel);
  return true;
}

bool cIptvDevice::SetPid(cPidHandle *Handle, int Type, bool On)
{
  debug("cIptvDevice::SetPid(%d) Pid=%d Type=%d On=%d\n", deviceIndex, Handle->pid, Type, On);
  return true;
}

bool cIptvDevice::DeleteFilter(unsigned int Index)
{
  if ((Index < eMaxSecFilterCount) && secfilters[Index]) {
     //debug("cIptvDevice::DeleteFilter(%d) Index=%d\n", deviceIndex, Index);
     cIptvSectionFilter *tmp = secfilters[Index];
     secfilters[Index] = NULL;
     delete tmp;
     return true;
     }
  return false;
}

bool cIptvDevice::IsBlackListed(u_short Pid, u_char Tid, u_char Mask)
{
  //debug("cIptvDevice::IsBlackListed(%d) Pid=%d Tid=%02X Mask=%02X\n", deviceIndex, Pid, Tid, Mask);
  // loop through section filter table
  for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i) {
      int index = IptvConfig.GetDisabledFilters(i);
      // check if matches
      if ((index >= 0) && (index < SECTION_FILTER_TABLE_SIZE) &&
          (section_filter_table[index].pid == Pid) && (section_filter_table[index].tid == Tid) &&
          (section_filter_table[index].mask == Mask)) {
         //debug("cIptvDevice::IsBlackListed(%d) Found=%s\n", deviceIndex, section_filter_table[index].description);
         return true;
         }
      }
  return false;
}

int cIptvDevice::OpenFilter(u_short Pid, u_char Tid, u_char Mask)
{
  // Check if disabled by user
  if (!IptvConfig.GetSectionFiltering())
     return -1;
  // Blacklist check, refuse certain filters
  if (IsBlackListed(Pid, Tid, Mask))
     return -1;
  // Search the next free filter slot
  for (unsigned int i = 0; i < eMaxSecFilterCount; ++i) {
      if (!secfilters[i]) {
         //debug("cIptvDevice::OpenFilter(%d): Pid=%d Tid=%02X Mask=%02X Index=%d\n", deviceIndex, Pid, Tid, Mask, i);
         secfilters[i] = new cIptvSectionFilter(i, deviceIndex, Pid, Tid, Mask);
         return secfilters[i]->GetReadDesc();
         }
      }
  // No free filter slot found
  return -1;
}

bool cIptvDevice::CloseFilter(int Handle)
{
  for (unsigned int i = 0; i < eMaxSecFilterCount; ++i) {
      if (secfilters[i] && (Handle == secfilters[i]->GetReadDesc())) {
         //debug("cIptvDevice::CloseFilter(%d): %d\n", deviceIndex, Handle);
         return DeleteFilter(i);
         }
      }
  return false;
}

bool cIptvDevice::OpenDvr(void)
{
  debug("cIptvDevice::OpenDvr(%d)\n", deviceIndex);
  mutex.Lock();
  isPacketDelivered = false;
  tsBuffer->Clear();
  mutex.Unlock();
  ResetBuffering();
  pIptvStreamer->Open();
  if (pSidScanner && IptvConfig.GetSectionFiltering() && IptvConfig.GetSidScanning())
     pSidScanner->SetStatus(true);
  isOpenDvr = true;
  return true;
}

void cIptvDevice::CloseDvr(void)
{
  debug("cIptvDevice::CloseDvr(%d)\n", deviceIndex);
  if (pSidScanner && IptvConfig.GetSectionFiltering() && IptvConfig.GetSidScanning())
     pSidScanner->SetStatus(false);
  pIptvStreamer->Close();
  isOpenDvr = false;
}

bool cIptvDevice::HasLock(int TimeoutMs)
{
  //debug("cIptvDevice::HasLock(%d): %d\n", deviceIndex, TimeoutMs);
  return (!IsBuffering());
}

void cIptvDevice::ResetBuffering(void)
{
  debug("cIptvDevice::ResetBuffering(%d)\n", deviceIndex);
  // pad prefill to multiple of TS_SIZE
  tsBufferPrefill = MEGABYTE(IptvConfig.GetTsBufferSize()) *
                    IptvConfig.GetTsBufferPrefillRatio() / 100;
  tsBufferPrefill -= (tsBufferPrefill % TS_SIZE);
}

bool cIptvDevice::IsBuffering(void)
{
  //debug("cIptvDevice::IsBuffering(%d): %d\n", deviceIndex);
  if (tsBufferPrefill && tsBuffer->Available() < tsBufferPrefill)
     return true;
  else
     tsBufferPrefill = 0;
  return false;
}

bool cIptvDevice::GetTSPacket(uchar *&Data)
{
  int Count = 0;
  //debug("cIptvDevice::GetTSPacket(%d)\n", deviceIndex);
  if (!IsBuffering()) {
     if (isPacketDelivered) {
        tsBuffer->Del(TS_SIZE);
        isPacketDelivered = false;
        }
     uchar *p = tsBuffer->Get(Count);
     if (p && Count >= TS_SIZE) {
        if (*p != TS_SYNC_BYTE) {
           for (int i = 1; i < Count; i++) {
               if (p[i] == TS_SYNC_BYTE) {
                  Count = i;
                  break;
                  }
               }
           tsBuffer->Del(Count);
           error("ERROR: skipped %d bytes to sync on TS packet\n", Count);
           return false;
           }
        isPacketDelivered = true;
        Data = p;
	// Update statistics 
	cIptvDeviceStatistics::AddStatistic(TS_SIZE, ts_pid(p), payload(p));
	cIptvBufferStatistics::AddStatistic(tsBuffer->Available(), tsBuffer->Free());
        // Run the data through all filters
        for (unsigned int i = 0; i < eMaxSecFilterCount; ++i) {
            if (secfilters[i])
               secfilters[i]->ProcessData(p);
            }
        return true;
        }
     }
  Data = NULL;
  return true;
}
