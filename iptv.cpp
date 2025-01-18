/*
 * iptv.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "iptv.h"
#include <getopt.h>
#include <vector>
#include "common.h"
#include "config.h"
#include "setup.h"
#include "device.h"
#include "iptvservice.h"
#include "source.h"

#if defined(APIVERSNUM) && APIVERSNUM < 20400
#error "VDR-2.4.0 API version or greater is required!"
#endif

#ifndef GITVERSION
#define GITVERSION ""
#endif

const char VERSION[] = "2.6.6" GITVERSION;
static const char DESCRIPTION[] = trNOOP("Experience the IPTV");

const char *cPluginIptv::Description() { return tr(DESCRIPTION); }

std::thread thread404;
bool thread404Running;

void run404() {
    while(thread404Running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        rename404Channels();
    }
}

cPluginIptv::cPluginIptv() : deviceCountM(1) {
    debug16("%s", __PRETTY_FUNCTION__);
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginIptv::~cPluginIptv() {
    debug16("%s", __PRETTY_FUNCTION__);
    // Clean up after yourself!
    if (thread404.joinable()) {
        thread404Running = false;
        thread404.join();
    }
}

const char *cPluginIptv::CommandLineHelp() {
    debug1("%s", __PRETTY_FUNCTION__);
    // Return a string that describes all known command line options.
    return "  -d <num>,  --devices=<number>           number of devices to be created\n"
           "  -t <mode>, --trace=<mode>               set the tracing mode\n"
           "  -s <num>,  --thread-queue-size=<number> set the FFmpeg thread-queue-size\n"
           "  -y <path>, --ytdlp=<path>               set the path to yt-dlp. Default /usr/local/bin/yt-dlp\n"
           "  -m <path>, --m3u-config-path=<path>     sets the path to m3u cfg files\n";
}

bool cPluginIptv::ProcessArgs(int argc, char *argv[]) {
    debug1("%s", __PRETTY_FUNCTION__);

    // Implement command line argument processing here if applicable.
    static const struct option long_options[] = {
        {"devices", required_argument, nullptr, 'd'},
        {"trace", required_argument, nullptr, 't'},
        {"thread-queue-size", required_argument, nullptr, 's'},
        {"ytdlp", required_argument, nullptr, 'y'},
        {"m3u-config-path", required_argument, nullptr, 'm'},
        {nullptr, no_argument, nullptr, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "d:t:s:y:m:", long_options, nullptr))!=-1) {
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
            IptvConfig.SetYtdlpPath(optarg);
            break;

        case 'm':
            IptvConfig.SetM3uCfgPath(optarg);
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

    // set some default values
    if (strlen(IptvConfig.GetM3uCfgPath()) == 0) {
        IptvConfig.SetM3uCfgPath(cPlugin::ResourceDirectory(PLUGIN_NAME_I18N));
    }

    return cIptvDevice::Initialize(deviceCountM);
}

bool cPluginIptv::Start() {
    debug1("%s", __PRETTY_FUNCTION__);
    // Start any background activities the plugin shall perform.
    if (curl_global_init(CURL_GLOBAL_ALL)==CURLE_OK) {
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

    thread404Running = true;
    thread404 = std::thread(run404);

    return true;
}

void cPluginIptv::Stop() {
    debug1("%s", __PRETTY_FUNCTION__);
    // Stop any background activities the plugin is performing.
    cIptvDevice::Shutdown();
    curl_global_cleanup();

    thread404Running = false;
    thread404.join();
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
        if ((valueP = strchr(valueP, ' '))!=nullptr)
            valueP++;
    }
    return n;
}

bool cPluginIptv::SetupParse(const char *nameP, const char *valueP) {
    debug1("%s (%s, %s)", __PRETTY_FUNCTION__, nameP, valueP);
    // Parse your own setup parameters and store their values.
    if (!strcasecmp(nameP, "ExtProtocolBasePort")) {
        IptvConfig.SetProtocolBasePort(atoi(valueP));
    } else if (!strcasecmp(nameP, "SectionFiltering")) {
        IptvConfig.SetSectionFiltering(atoi(valueP));
    } else if (!strcasecmp(nameP, "DisabledFilters")) {
        int DisabledFilters[SECTION_FILTER_TABLE_SIZE];
        for (unsigned int i = 0; i < ARRAY_SIZE(DisabledFilters); ++i) {
            DisabledFilters[i] = -1;
        }
        unsigned int DisabledFiltersCount = ParseFilters(valueP, DisabledFilters);
        for (unsigned int i = 0; i < DisabledFiltersCount; ++i) {
            IptvConfig.SetDisabledFilters(i, DisabledFilters[i]);
        }
    } else if (!strcasecmp(nameP, "StillPicture")) {
        IptvConfig.SetStillPicture(atoi(valueP));
    } else if (!strcasecmp(nameP, "EnableEpgScan")) {
        IptvConfig.SetEnableEpgScan(atoi(valueP));
    } else {
        return false;
    }
    return true;
}

bool cPluginIptv::Service(const char *idP, void *dataP) {
    debug1("%s (%s, )", __PRETTY_FUNCTION__, idP);
    if (strcmp(idP, "IptvService-v1.0")==0) {
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
        "LXML\n"
        "    List all xmltv ids in channels.conf.\n",
        "LIPT [ <type> ]\n"
        "    List all iptv channels of <type>.\n",
        "CHPA <number> <name> <value>\n"
        "    Change the parameter with name <name> to <value> of channel with number <number>\n",
        "ADDM <cfg file>#<name>#<URL>\n"
        "    adds a M3U URL to the config file and channels. PIDs and frequency will be automatically set\n",
        "ADDS <name>#<URL>\n"
        "    adds a STREAM URL to channels. PIDs and frequency will be automatically set\n",
        "ADDR <name>#<URL>\n"
        "    adds a RADIO URL to channels. PIDs and frequency will be automatically set\n",
        "CHKU START, STATUS, STOP\n"
        "    checks all configured URL and checks if a connect is possible and a valid result is returned.\n"
        "    If this is not the case the channel will be marked with ' - 404'\n",
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
            return {"IPTV information not available!"};
        }
    } else if (strcasecmp(commandP, "MODE") == 0) {
        unsigned int mode = !IptvConfig.GetUseBytes();
        IptvConfig.SetUseBytes(mode);
        return cString::sprintf("IPTV information mode is: %s\n", mode ? "bytes" : "bits");
    } else if (strcasecmp(commandP, "TRAC") == 0) {
        if (optionP && *optionP)
            IptvConfig.SetTraceMode(strtol(optionP, nullptr, 0));
        return cString::sprintf("IPTV tracing mode: 0x%04X\n", IptvConfig.GetTraceMode());
    } else if (strcasecmp(commandP, "LXML") == 0) {
        LOCK_CHANNELS_READ;
        std::string result;
        for (const cChannel *Channel = Channels->First(); Channel; Channel = Channels->Next(Channel)) {
            if (Channel->IsSourceType('I')) {
                cIptvTransponderParameters params(Channel->Parameters());
                if (!params.XmlTVId().empty()) {
                    result.append(std::to_string(Channel->Number())).append(" ").append(Channel->GetChannelID().ToString()).append(":").append(Channel->Name()).append(":").append(
                        params.XmlTVId()).append("\n");
                }
            }
        }

        if (result.empty()) {
            replyCodeP = 550;
            return { "No channels with xmltv id found\n" };
        }

        replyCodeP = 250;
        return { result.c_str() };
    } else if (strcasecmp(commandP, "LIPT") == 0) {
        int type = -1;

        if (*optionP) {
            std::string typeStr = std::string(optionP);
            trim(typeStr);
            type = cIptvTransponderParameters::StrToProtocol(typeStr.c_str());
        }

        LOCK_CHANNELS_READ;
        std::string result;
        for (const cChannel *Channel = Channels->First(); Channel; Channel = Channels->Next(Channel)) {
            if (Channel->IsSourceType('I')) {
                cIptvTransponderParameters params(Channel->Parameters());

                if (type == -1 || params.Protocol() == type) {
                    result.append(std::to_string(Channel->Number())).append(" ").append(Channel->GetChannelID().ToString()).append(":").append(Channel->Name()).append(":").append(cIptvTransponderParameters::ProtocolToStr(params.Protocol())).append("\n");
                }
            }
        }

        if (result.empty()) {
            replyCodeP = 550;
            return { "No iptv channels found\n" };
        }

        replyCodeP = 250;
        return { result.c_str() };
    } else if (strcasecmp(commandP, "CHPA") == 0) {
        int number = -1;
        std::string name;
        std::string value;

        if (*optionP) {
            char buf[strlen(optionP) + 1];
            char *p = strcpy(buf, optionP);
            char *strtok_next = nullptr;

            if ((p = strtok_r(p, " \t", &strtok_next))!=nullptr) {
                if (isnumber(p)) {
                    number = atoi(p);
                } else {
                    replyCodeP = 501;
                    return {"Invalid channel number"};
                }

                if ((p = strtok_r(nullptr, " \t", &strtok_next))!=nullptr) {
                    name = p;
                } else {
                    replyCodeP = 501;
                    return {"Invalid parameter name"};
                }

                if ((p = strtok_r(nullptr, " \t", &strtok_next))!=nullptr) {
                    value = p;
                } else {
                    replyCodeP = 501;
                    return {"Invalid parameter value"};
                }
            }
        } else {
            replyCodeP = 501;
            return { "Missing parameters" };
        }

        LOCK_CHANNELS_WRITE;
        cChannel *Channel = Channels->GetByNumber(number);
        if (Channel->IsSourceType('I')) {
            cIptvTransponderParameters params(Channel->Parameters());

            if (name == "S") {
                if ((value == "0") || (value == "1")) {
                    params.SetSidScan(atoi(value.c_str()));
                } else {
                    replyCodeP = 501;
                    return { "Only 1 or 0 are allowed for parameter S" };
                }
            } else if (name == "P") {
                if ((value == "0") || (value == "1")) {
                    params.SetPidScan(atoi(value.c_str()));
                } else {
                    replyCodeP = 501;
                    return { "Only 1 or 0 are allowed for parameter P" };
                }
            } else if (name == "F") {
                int prot = cIptvTransponderParameters::StrToProtocol(value.c_str());
                if (prot == -1) {
                    replyCodeP = 501;
                    return { "Value for parameter F is illegal" };
                }
                params.SetProtocol(prot);
            } else if (name == "U") {
                std::string u = value.c_str();
                u = ReplaceAll(u, ":", "%3A");
                u = ReplaceAll(u, "|", "%7C");
                params.SetAddress(u.c_str());
            } else if (name == "A") {
                params.SetParameter(atoi(value.c_str()));
            } else if (name == "Y") {
                if ((value ==  "1") || (value == "2")) {
                    params.SetYtdlp(atoi(value.c_str()));
                } else {
                    replyCodeP = 501;
                    return { "Only 1 or 2 are allowed for parameter Y" };
                }
            } else if (name == "H") {
                if ((value == "F") || (value == "V")) {
                    params.SetHandlerType(value.at(0));
                } else {
                    replyCodeP = 501;
                    return { "Only F or H are allowed for parameter F" };
                }
            } else if (name == "X") {
                params.SetXmlTvId(value);
            } else {
                replyCodeP = 501;
                return { "Parameter name is illegal" };
            }

            // Update channel
            Channel->SetTransponderData(Channel->Source(), Channel->Frequency(), Channel->Srate(), params.ToString('I').c_str());
        } else {
            replyCodeP = 501;
            return { "Channel is not an iptv channel" };
        }

        replyCodeP = 250;
        return { "OK" };
    } else if (strcasecmp(commandP, "ADDM") == 0) {
        if (*optionP) {
            char buf[strlen(optionP) + 1];
            char *p = strcpy(buf, optionP);
            char *strtok_next = nullptr;

            std::string cfgFile;
            std::string name;
            std::string url;

            if ((p = strtok_r(p, "#", &strtok_next)) != nullptr) {
                cfgFile = p;

                if ((p = strtok_r(nullptr, "#", &strtok_next)) != nullptr) {
                    name = p;

                    if ((p = strtok_r(nullptr, "#", &strtok_next)) != nullptr) {
                        url = p;
                    } else {
                        replyCodeP = 501;
                        return { "parameter <url> not found" };
                    }
                } else {
                    replyCodeP = 501;
                    return { "parameter <name> not found" };
                }
            } else {
                replyCodeP = 501;
                return { "parameter <cfg file> not found" };
            }

            int id = addM3UCfg(cfgFile, url);
            if (id < 0) {
                replyCodeP = 501;
                return cString::sprintf("Unable to open cfg file %s\n", cfgFile.c_str());
            }

            // calculate Frequence and PIDs
            unsigned long freq;
            unsigned int sid;
            unsigned int tid;
            unsigned int rid;
            findFreeFreqPid(freq, sid, tid, rid);

            // create channels.conf entry
            cString ch = cString::sprintf("%s:%lu:S=1|P=0|F=M3U|U=%s|A=%d:I:0:256:257:0:0:%u:1:%u:0", name.c_str(), freq, cfgFile.c_str(), id, sid, tid);
            cString replyMessage;
            addChannel(ch, replyCodeP, replyMessage);

            return replyMessage;
        }
    } else if (strcasecmp(commandP, "ADDS") == 0) {
        if (*optionP) {
            char buf[strlen(optionP) + 1];
            char *p = strcpy(buf, optionP);
            char *strtok_next = nullptr;

            std::string name;
            std::string url;

            if ((p = strtok_r(p, "#", &strtok_next)) != nullptr) {
                name = p;

                if ((p = strtok_r(nullptr, "#", &strtok_next)) != nullptr) {
                    url = p;
                    url = ReplaceAll(url, ":", "%3A");
                    url = ReplaceAll(url, "|", "%7C");
                } else {
                    replyCodeP = 501;
                    return { "parameter <url> not found" };
                }
            } else {
                replyCodeP = 501;
                return { "parameter <name> not found" };
            }

            // calculate Frequence and PIDs
            unsigned long freq;
            unsigned int sid;
            unsigned int tid;
            unsigned int rid;
            findFreeFreqPid(freq, sid, tid, rid);

            // create channels.conf entry
            cString ch = cString::sprintf("%s:%lu:S=1|P=0|F=STREAM|U=%s|A=1:I:0:256:257:0:0:%u:1:%u:0", name.c_str(), freq, url.c_str(), sid, tid);
            cString replyMessage;
            addChannel(ch, replyCodeP, replyMessage);

            return replyMessage;
        }
    } else if (strcasecmp(commandP, "ADDR") == 0) {
        if (*optionP) {
            char buf[strlen(optionP) + 1];
            char *p = strcpy(buf, optionP);
            char *strtok_next = nullptr;

            std::string name;
            std::string url;

            if ((p = strtok_r(p, "#", &strtok_next)) != nullptr) {
                name = p;

                if ((p = strtok_r(nullptr, "#", &strtok_next)) != nullptr) {
                    url = p;
                    url = ReplaceAll(url, ":", "%3A");
                    url = ReplaceAll(url, "|", "%7C");
                } else {
                    replyCodeP = 501;
                    return { "parameter <url> not found" };
                }
            } else {
                replyCodeP = 501;
                return { "parameter <name> not found" };
            }

            // calculate Frequence and PIDs
            unsigned long freq;
            unsigned int sid;
            unsigned int tid;
            unsigned int rid;
            findFreeFreqPid(freq, sid, tid, rid);

            // create channels.conf entry
            cString ch = cString::sprintf("%s:%lu:S=1|P=0|F=RADIO|U=%s|A=1:I:0:0:257:0:0:%u:1:%u:%u", name.c_str(), freq, url.c_str(), sid, tid, rid);
            cString replyMessage;
            addChannel(ch, replyCodeP, replyMessage);

            return replyMessage;
        }
    } else if (strcasecmp(commandP, "CHKU") == 0) {
        if (*optionP) {
            char buf[strlen(optionP) + 1];
            char *p = strcpy(buf, optionP);

            cString cmd = cString(p);
            cmd = ChangeCase(cmd, true);

            if (strcmp(cmd, "START") == 0)  {
                chk.start();
                replyCodeP = 250;
                return { "URL check started" };
            } else if (strcmp(cmd, "STATUS") == 0) {
                replyCodeP = 250;
                return  { chk.status() };
            } else if (strcmp(cmd, "STOP") == 0) {
                chk.stop();
                replyCodeP = 250;
                return { "URL check stopped" };
            } else {
                replyCodeP = 501;
                return { "CHKU command is unknown" };
            }
        } else {
            replyCodeP = 501;
            return { "CHKU missing command START, STATUS or STOP" };
        }
    }

    return nullptr;
}
void cPluginIptv::findFreeFreqPid(unsigned long &freq, unsigned int &sid, unsigned int &tid, unsigned int &rid) {
    std::vector<unsigned long> freqs;
    std::set<unsigned int> sids;
    std::set<unsigned int> tids;
    std::set<unsigned int> rids;

    // collect all data
    {
        LOCK_CHANNELS_READ;
        std::string result;
        for (const cChannel *Channel = Channels->First(); Channel; Channel = Channels->Next(Channel)) {
            if (Channel->IsSourceType('I')) {
                freqs.emplace_back(Channel->Frequency());
                sids.insert(Channel->Sid());
                tids.insert(Channel->Tid());
                rids.insert(Channel->Rid());
            }
        }
    }

    // Calculate free pids
    unsigned long i;

    for (i = 1; i < 65535 && sids.find(i) != sids.end(); ++i);
    sid = i;

    for (i = 1; i < 65535 && tids.find(i) != tids.end(); ++i);
    tid = i;

    for (i = 1; i < 65535 && rids.find(i) != rids.end(); ++i);
    rid = i;

    // calculate free frequency
    unsigned long f;
    std::sort(freqs.begin(), freqs.end());

    std::vector<unsigned long> diffs;
    for (i = 1; i < freqs.size(); ++i) {
        diffs.emplace_back(freqs.at(i) - freqs.at(i-1));
    }

    f = freqs.at(freqs.size()-1) + 10;
    for (i = 0; i < diffs.size(); ++i) {
        if (diffs.at(i) > 20)  {
            f = freqs.at(i) + 10;
            break;
        }
    }

    freq = f;
}

void cPluginIptv::addChannel(cString &channelStr, int &replyCode, cString &replyMessage) {
    cChannel channel;
    if (channel.Parse(channelStr)) {
        LOCK_CHANNELS_WRITE;
        Channels->SetExplicitModify();
        if (Channels->HasUniqueChannelID(&channel)) {
            cChannel *nchannel = new cChannel;
            *nchannel = channel;
            Channels->Add(nchannel);
            Channels->ReNumber();
            Channels->SetModifiedByUser();
            Channels->SetModified();

            replyCode = 250;
            replyMessage = cString::sprintf("%d %s", nchannel->Number(), *nchannel->ToText());
        } else {
            replyCode = 501;
            replyMessage = cString::sprintf("Channel settings are not unique");
        }
    } else {
        replyCode = 501;
        replyMessage = cString::sprintf("Error in channel settings");
    }
}
int cPluginIptv::addM3UCfg(const std::string& cfgFile, const std::string& url) {
    int maxNr = -1;

    cString configFile = cString::sprintf("%s/%s", IptvConfig.GetM3uCfgPath(), cfgFile.c_str());

    std::ifstream file(configFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            unsigned long idx = line.find(':');
            if (idx > 0) {
                int nr = std::atoi(line.substr(0, idx).c_str());

                if (maxNr <= nr) {
                    maxNr = nr + 1;
                }
            }
        }

        file.close();
    } else {
        maxNr = 1;
    }

    std::ofstream outfile;
    outfile.open(configFile, std::ios_base::app);
    if (outfile.is_open()) {
        outfile << maxNr << ":" << url << "\n";
        outfile.close();
    } else {
        maxNr = -1;
    }

    return maxNr;
}

VDRPLUGINCREATOR(cPluginIptv); // Don't touch this!
