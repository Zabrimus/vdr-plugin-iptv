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

static const char DESCRIPTION[] = trNOOP("Experience the IPTV");

class cPluginIptv : public cPlugin {
private:
    unsigned int deviceCountM;
    int ParseFilters(const char *valueP, int *filtersP);

public:
    cPluginIptv();
    ~cPluginIptv() override;

    const char *Version() override { return VERSION; }
    const char *Description() override { return tr(DESCRIPTION); }
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
