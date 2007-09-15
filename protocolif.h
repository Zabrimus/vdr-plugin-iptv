/*
 * protocolif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolif.h,v 1.2 2007/09/15 20:33:15 rahrenbe Exp $
 */

#ifndef __IPTV_PROTOCOLIF_H
#define __IPTV_PROTOCOLIF_H

class cIptvProtocolIf {
public:
  cIptvProtocolIf() {}
  virtual ~cIptvProtocolIf() {}
  virtual int Read(unsigned char *Buffer) = 0;
  virtual bool Set(const char* Address, const int Port) = 0;
  virtual bool Open(void) = 0;
  virtual bool Close(void) = 0;

private:
    cIptvProtocolIf(const cIptvProtocolIf&);
    cIptvProtocolIf& operator=(const cIptvProtocolIf&);
};

#endif // __IPTV_PROTOCOLIF_H
