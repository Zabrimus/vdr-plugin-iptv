/*
 * iptvservice.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once
#include <vdr/tools.h>

#define stIptv ('I' << 24)

struct IptvService_v1_0 {
    int cardIndex;
    cString protocol;
    cString bitrate;
};

