/*
 * sidscanner.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <vdr/channels.h>
#include <vdr/filter.h>

#include "log.h"

class cSidScanner : public cFilter {
private:
    tChannelID channelIdM;
    bool sidFoundM;
    bool nidFoundM;
    bool tidFoundM;
    bool isActiveM;

protected:
    void Process(u_short pidP, u_char tidP, const u_char *dataP, int lengthP) override;

public:
    cSidScanner();
    ~cSidScanner() override;
    void SetChannel(const tChannelID &channelIdP);
    void Open() {
        debug1("%s", __PRETTY_FUNCTION__);

        isActiveM = true;
    }
    void Close() {
        debug1("%s", __PRETTY_FUNCTION__);

        isActiveM = false;
    }
};
