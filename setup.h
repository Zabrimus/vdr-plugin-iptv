/*
 * setup.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <vdr/menuitems.h>
#include <vdr/sourceparams.h>
#include "common.h"

class cIptvPluginSetup : public cMenuSetupPage {
private:
    int protocolBasePortM;
    int sectionFilteringM;
    int numDisabledFiltersM;
    int disabledFilterIndexesM[SECTION_FILTER_TABLE_SIZE];
    const char *disabledFilterNamesM[SECTION_FILTER_TABLE_SIZE];
    cVector<const char *> helpM;

    eOSState ShowInfo();
    void Setup();
    void StoreFilters(const char *nameP, int *valuesP);

protected:
    eOSState ProcessKey(eKeys keyP) override;
    void Store() override;

public:
    cIptvPluginSetup();
};
