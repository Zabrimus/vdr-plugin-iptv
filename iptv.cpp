/*
 * iptv.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "iptv.h"
#include <getopt.h>
#include "common.h"
#include "config.h"
#include "setup.h"
#include "device.h"
#include "iptvservice.h"

#if defined(APIVERSNUM) && APIVERSNUM < 20400
#error "VDR-2.4.0 API version or greater is required!"
#endif

#ifndef GITVERSION
#define GITVERSION ""
#endif

const char VERSION[] = "2.4.0" GITVERSION;
static const char DESCRIPTION[] = trNOOP("Experience the IPTV");

const char *cPluginIptv::Description() { return tr(DESCRIPTION); }

cPluginIptv::cPluginIptv() : deviceCountM(1) {
    debug16("%s", __PRETTY_FUNCTION__);
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginIptv::~cPluginIptv() {
    debug16("%s", __PRETTY_FUNCTION__);
    // Clean up after yourself!
}

const char *cPluginIptv::CommandLineHelp(void) {
    debug1("%s", __PRETTY_FUNCTION__);
    // Return a string that describes all known command line options.
    return "  -d <num>,  --devices=<number>           number of devices to be created\n"
           "  -t <mode>, --trace=<mode>               set the tracing mode\n"
           "  -s <num>,  --thread-queue-size=<number> set the FFmpeg thread-queue-size\n"
           "  -y <path>, --ytdlp=<path>               set the path to yt-dlp. Default /usr/local/bin/yt-dlp\n";
}

bool cPluginIptv::ProcessArgs(int argc, char *argv[]) {
    debug1("%s", __PRETTY_FUNCTION__);
    // Implement command line argument processing here if applicable.
    static const struct option long_options[] = {
            {"devices",           required_argument, nullptr, 'd'},
            {"trace",             required_argument, nullptr, 't'},
            {"thread-queue-size", required_argument, nullptr, 's'},
            {"ytdlp",             required_argument, nullptr, 'y'},
            {nullptr,             no_argument,       nullptr, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "d:t:s:y:", long_options, nullptr)) != -1) {
        switch (c) {
            case 'd':
                deviceCountM = atoi(optarg);
                break;
            case 't':
                IptvConfig.SetTraceMode(strtol(optarg, nullptr, 0));
                break;
            case 's':
                IptvConfig.SetThreadQueueSize(strtol(optarg, nullptr, 0));
                break;
            case 'y':
                IptvConfig.SetYtdlpPath(std::string(optarg));
                break;
            default:
                return false;
        }
    }
    return true;
}

bool cPluginIptv::Initialize() {
    debug1("%s", __PRETTY_FUNCTION__);
    // Initialize any background activities the plugin shall perform.
    IptvConfig.SetConfigDirectory(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N));
    IptvConfig.SetResourceDirectory(cPlugin::ResourceDirectory(PLUGIN_NAME_I18N));
    return cIptvDevice::Initialize(deviceCountM);
}

bool cPluginIptv::Start() {
    debug1("%s", __PRETTY_FUNCTION__);
    // Start any background activities the plugin shall perform.
    if (curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK) {
        curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);
        cString info = cString::sprintf("Using CURL %s", data->version);
        for (int i = 0; data->protocols[i]; ++i) {
            // Supported protocols: HTTP(S), RTSP, FILE
            if (startswith(data->protocols[i], "http") || startswith(data->protocols[i], "rtsp") ||
                startswith(data->protocols[i], "file"))
                info = cString::sprintf("%s %s", *info, data->protocols[i]);
        }
        info("%s", *info);
    }

    return true;
}

void cPluginIptv::Stop() {
    debug1("%s", __PRETTY_FUNCTION__);
    // Stop any background activities the plugin is performing.
    cIptvDevice::Shutdown();
    curl_global_cleanup();
}

void cPluginIptv::Housekeeping() {
    debug16("%s", __PRETTY_FUNCTION__);
    // Perform any cleanup or other regular tasks.
}

void cPluginIptv::MainThreadHook() {
    debug16("%s", __PRETTY_FUNCTION__);
    // Perform actions in the context of the main program thread.
    // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginIptv::Active() {
    debug16("%s", __PRETTY_FUNCTION__);
    // Return a message string if shutdown should be postponed
    return nullptr;
}

time_t cPluginIptv::WakeupTime() {
    debug16("%s", __PRETTY_FUNCTION__);
    // Return custom wakeup time for shutdown script
    return 0;
}

cOsdObject *cPluginIptv::MainMenuAction() {
    debug16("%s", __PRETTY_FUNCTION__);
    // Perform the action when selected from the main VDR menu.
    return nullptr;
}

cMenuSetupPage *cPluginIptv::SetupMenu() {
    debug1("%s", __PRETTY_FUNCTION__);
    // Return a setup menu in case the plugin supports one.
    return new cIptvPluginSetup();
}

int cPluginIptv::ParseFilters(const char *valueP, int *filtersP) {
    debug1("%s (%s, )", __PRETTY_FUNCTION__, valueP);
    char buffer[256];
    int n = 0;
    while (valueP && *valueP && (n < SECTION_FILTER_TABLE_SIZE)) {
        strn0cpy(buffer, valueP, sizeof(buffer));
        int i = atoi(buffer);
        debug16("%s (%s, ) filters[%d]=%d", __PRETTY_FUNCTION__, valueP, n, i);
        if (i >= 0)
            filtersP[n++] = i;
        if ((valueP = strchr(valueP, ' ')) != nullptr)
            valueP++;
    }
    return n;
}

bool cPluginIptv::SetupParse(const char *nameP, const char *valueP) {
    debug1("%s (%s, %s)", __PRETTY_FUNCTION__, nameP, valueP);
    // Parse your own setup parameters and store their values.
    if (!strcasecmp(nameP, "ExtProtocolBasePort"))
        IptvConfig.SetProtocolBasePort(atoi(valueP));
    else if (!strcasecmp(nameP, "SectionFiltering"))
        IptvConfig.SetSectionFiltering(atoi(valueP));
    else if (!strcasecmp(nameP, "DisabledFilters")) {
        int DisabledFilters[SECTION_FILTER_TABLE_SIZE];
        for (unsigned int i = 0; i < ARRAY_SIZE(DisabledFilters); ++i)
            DisabledFilters[i] = -1;
        unsigned int DisabledFiltersCount = ParseFilters(valueP, DisabledFilters);
        for (unsigned int i = 0; i < DisabledFiltersCount; ++i)
            IptvConfig.SetDisabledFilters(i, DisabledFilters[i]);
    } else
        return false;
    return true;
}

bool cPluginIptv::Service(const char *idP, void *dataP) {
    debug1("%s (%s, )", __PRETTY_FUNCTION__, idP);
    if (strcmp(idP, "IptvService-v1.0") == 0) {
        if (dataP) {
            IptvService_v1_0 *data = reinterpret_cast<IptvService_v1_0 *>(dataP);
            cIptvDevice *dev = cIptvDevice::GetIptvDevice(data->cardIndex);
            if (!dev)
                return false;
            data->protocol = dev->GetInformation(IPTV_DEVICE_INFO_PROTOCOL);
            data->bitrate = dev->GetInformation(IPTV_DEVICE_INFO_BITRATE);
        }
        return true;
    }
    return false;
}

const char **cPluginIptv::SVDRPHelpPages() {
    debug1("%s", __PRETTY_FUNCTION__);
    static const char *HelpPages[] = {
            "INFO [ <page> ]\n"
            "    Print IPTV device information and statistics.\n"
            "    The output can be narrowed using optional \"page\""
            "    option: 1=general 2=pids 3=section filters.\n",
            "MODE\n"
            "    Toggles between bit or byte information mode.\n",
            "TRAC [ <mode> ]\n"
            "    Gets and/or sets used tracing mode.\n",
            nullptr
    };
    return HelpPages;
}

cString cPluginIptv::SVDRPCommand(const char *commandP, const char *optionP, int &replyCodeP) {
    debug1("%s (%s, %s, )", __PRETTY_FUNCTION__, commandP, optionP);
    if (strcasecmp(commandP, "INFO") == 0) {
        cIptvDevice *device = cIptvDevice::GetIptvDevice(cDevice::ActualDevice()->CardIndex());
        if (device) {
            int page = IPTV_DEVICE_INFO_ALL;
            if (optionP) {
                page = atoi(optionP);
                if ((page < IPTV_DEVICE_INFO_ALL) || (page > IPTV_DEVICE_INFO_FILTERS))
                    page = IPTV_DEVICE_INFO_ALL;
            }
            return device->GetInformation(page);
        } else {
            replyCodeP = 550; // Requested action not taken
            return cString("IPTV information not available!");
        }
    } else if (strcasecmp(commandP, "MODE") == 0) {
        unsigned int mode = !IptvConfig.GetUseBytes();
        IptvConfig.SetUseBytes(mode);
        return cString::sprintf("IPTV information mode is: %s\n", mode ? "bytes" : "bits");
    } else if (strcasecmp(commandP, "TRAC") == 0) {
        if (optionP && *optionP)
            IptvConfig.SetTraceMode(strtol(optionP, nullptr, 0));
        return cString::sprintf("IPTV tracing mode: 0x%04X\n", IptvConfig.GetTraceMode());
    }

    return nullptr;
}

VDRPLUGINCREATOR(cPluginIptv); // Don't touch this!
