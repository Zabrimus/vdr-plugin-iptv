/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_CONFIG_H
#define __IPTV_CONFIG_H

#include <vdr/menuitems.h>
#include "common.h"

class cIptvConfig
{
private:
  unsigned int traceModeM;
  unsigned int protocolBasePortM;
  unsigned int useBytesM;
  unsigned int sectionFilteringM;
  unsigned int threadQueueSize;
  int disabledFiltersM[SECTION_FILTER_TABLE_SIZE];
  char configDirectoryM[PATH_MAX];
  char resourceDirectoryM[PATH_MAX];
  std::string ytdlpPath;

public:
  enum eTraceMode {
    eTraceModeNormal  = 0x0000,
    eTraceModeDebug1  = 0x0001,
    eTraceModeDebug2  = 0x0002,
    eTraceModeDebug3  = 0x0004,
    eTraceModeDebug4  = 0x0008,
    eTraceModeDebug5  = 0x0010,
    eTraceModeDebug6  = 0x0020,
    eTraceModeDebug7  = 0x0040,
    eTraceModeDebug8  = 0x0080,
    eTraceModeDebug9  = 0x0100,
    eTraceModeDebug10 = 0x0200,
    eTraceModeDebug11 = 0x0400,
    eTraceModeDebug12 = 0x0800,
    eTraceModeDebug13 = 0x1000,
    eTraceModeDebug14 = 0x2000,
    eTraceModeDebug15 = 0x4000,
    eTraceModeDebug16 = 0x8000,
    eTraceModeMask    = 0xFFFF
  };
  cIptvConfig();
  unsigned int GetTraceMode() const { return traceModeM; };
  unsigned int GetThreadQueueSize() const { return threadQueueSize; };
  bool IsTraceMode(eTraceMode modeP) const { return (traceModeM & modeP); };
  unsigned int GetProtocolBasePort() const { return protocolBasePortM; };
  unsigned int GetUseBytes() const { return useBytesM; };
  unsigned int GetSectionFiltering() const { return sectionFilteringM; };
  const char *GetConfigDirectory() const { return configDirectoryM; };
  const char *GetResourceDirectory() const { return resourceDirectoryM; };
  unsigned int GetDisabledFiltersCount() const;
  std::string GetYtdlpPath() { return ytdlpPath; };
  int GetDisabledFilters(unsigned int indexP) const;

  void SetTraceMode(unsigned int modeP) { traceModeM = (modeP & eTraceModeMask); };
  void SetThreadQueueSize(unsigned int tqs) { threadQueueSize = tqs; };
  void SetProtocolBasePort(unsigned int portNumberP) { protocolBasePortM = portNumberP; };
  void SetUseBytes(unsigned int onOffP) { useBytesM = onOffP; };
  void SetSectionFiltering(unsigned int onOffP) { sectionFilteringM = onOffP; };
  void SetDisabledFilters(unsigned int indexP, int numberP);
  void SetConfigDirectory(const char *directoryP);
  void SetResourceDirectory(const char *directoryP);
  void SetYtdlpPath(std::string p) { ytdlpPath = p; };
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
