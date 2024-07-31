/*
 * deviceif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

class cIptvDeviceIf {
public:
    cIptvDeviceIf() = default;
    virtual ~cIptvDeviceIf() = default;

    virtual void WriteData(u_char *bufferP, int lengthP) = 0;
    virtual unsigned int CheckData() = 0;

private:
    cIptvDeviceIf(const cIptvDeviceIf &);
    cIptvDeviceIf &operator=(const cIptvDeviceIf &);
};
