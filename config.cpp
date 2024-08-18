/*
 * config.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "log.h"
#include "config.h"

cIptvConfig IptvConfig;

cIptvConfig::cIptvConfig()
    : traceModeM(eTraceModeNormal),
      protocolBasePortM(4321),
      useBytesM(1),
      sectionFilteringM(1),
      threadQueueSize(32),
      stillPicture(0),
      ytdlpPath("/usr/local/bin/yt-dlp") {

    for (unsigned int i = 0; i < ARRAY_SIZE(disabledFiltersM); ++i) {
        disabledFiltersM[i] = -1;
    }

    memset(configDirectoryM, 0, sizeof(configDirectoryM));
    memset(resourceDirectoryM, 0, sizeof(resourceDirectoryM));
    memset(resourceDirectoryM, 0, sizeof(resourceDirectoryM));
    memset(m3uCfgPath, 0, sizeof(m3uCfgPath));
    memset(ytdlpPath, 0, sizeof(ytdlpPath));
}

unsigned int cIptvConfig::GetDisabledFiltersCount() const {
    unsigned int n = 0;

    while ((n < ARRAY_SIZE(disabledFiltersM) && (disabledFiltersM[n]!=-1))) {
        n++;
    }

    return n;
}

int cIptvConfig::GetDisabledFilters(unsigned int indexP) const {
    return (indexP < ARRAY_SIZE(disabledFiltersM)) ? disabledFiltersM[indexP] : -1;
}

void cIptvConfig::SetDisabledFilters(unsigned int indexP, int numberP) {
    if (indexP < ARRAY_SIZE(disabledFiltersM)) {
        disabledFiltersM[indexP] = numberP;
    }
}

void cIptvConfig::SetConfigDirectory(const char *directoryP) {
    debug1("%s (%s)", __PRETTY_FUNCTION__, directoryP);
    ERROR_IF(!realpath(directoryP, configDirectoryM), "Cannot canonicalize configuration directory");
}

void cIptvConfig::SetResourceDirectory(const char *directoryP) {
    debug1("%s (%s)", __PRETTY_FUNCTION__, directoryP);
    ERROR_IF(!realpath(directoryP, resourceDirectoryM), "Cannot canonicalize resource directory");
}

void cIptvConfig::SetYtdlpPath(const char *path) {
    debug1("%s (%s)", __PRETTY_FUNCTION__, path);
    ERROR_IF(!realpath(path, ytdlpPath), "Cannot canonicalize ytdlp path");

}

void cIptvConfig::SetM3uCfgPath(const char *directoryP) {
    debug1("%s (%s)", __PRETTY_FUNCTION__, directoryP);
    ERROR_IF(!realpath(directoryP, m3uCfgPath), "Cannot canonicalize m3ucfg directory");
}
