/*
 * iptv.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <getopt.h>
#include <vdr/plugin.h>
#include "common.h"
#include "config.h"
#include "setup.h"
#include "device.h"
#include "iptvservice.h"
#include "checkurl.h"

class cPluginIptv : public cPlugin {
private:
    unsigned int deviceCountM;
    int ParseFilters(const char *valueP, int *filtersP);

    void findFreeFreqPid(unsigned long &freq, unsigned int &sid, unsigned int &tid, unsigned int &rid);
    void addChannel(cString& channel, int &replyCode, cString& replyMessage);
    int addM3UCfg(const std::string& cfgFile, const std::string& url);

    CheckURL chk;

public:
    cPluginIptv();
    ~cPluginIptv() override;

    const char *Version() override { return VERSION; }
    const char *Description() override;
    const char *CommandLineHelp() override;

    bool ProcessArgs(int argc, char *argv[]) override;
    bool Initialize() override;

    bool Start() override;
    void Stop() override;

    void Housekeeping() override;
    void MainThreadHook() override;
    cString Active() override;
    time_t WakeupTime() override;

    const char *MainMenuEntry() override { return nullptr; }
    cOsdObject *MainMenuAction() override;

    cMenuSetupPage *SetupMenu() override;
    bool SetupParse(const char *Name, const char *Value) override;

    bool Service(const char *Id, void *Data) override;
    const char **SVDRPHelpPages() override;
    cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) override;
};
