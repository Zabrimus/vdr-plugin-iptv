/*
 * pidscanner.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "pidscanner.h"

#define PIDSCANNER_TIMEOUT_IN_MS   15000 /* 15s timeout for detection */
#define PIDSCANNER_APID_COUNT      5     /* minimum count of audio pid samples for pid detection */
#define PIDSCANNER_VPID_COUNT      5     /* minimum count of video pid samples for pid detection */
#define PIDSCANNER_PID_DELTA_COUNT 100   /* minimum count of pid samples for audio/video only pid detection */

cPidScanner::cPidScanner(void)
: timeoutM(0),
  channelIdM(tChannelID::InvalidID),
  processM(true),
  vPidM(0xFFFF),
  aPidM(0xFFFF),
  numVpidsM(0),
  numApidsM(0)
{
  debug("cPidScanner::%s()", __FUNCTION__);
}

cPidScanner::~cPidScanner()
{
  debug("cPidScanner::%s()", __FUNCTION__);
}

void cPidScanner::SetChannel(const tChannelID &channelIdP)
{
  debug("cPidScanner::%s(%s)", __FUNCTION__, *channelIdP.ToString());
  channelIdM = channelIdP;
  vPidM = 0xFFFF;
  numVpidsM = 0;
  aPidM = 0xFFFF;
  numApidsM = 0;
  processM = true;
  timeoutM.Set(PIDSCANNER_TIMEOUT_IN_MS);
}

void cPidScanner::Process(const uint8_t* bufP)
{
  //debug("cPidScanner::%s()", __FUNCTION__);
  if (!processM)
     return;

  // Stop scanning after defined timeout
  if (timeoutM.TimedOut()) {
     debug("cPidScanner::%s(): timed out determining pids", __FUNCTION__);
     processM = false;
  }

  // Verify TS packet
  if (bufP[0] != 0x47) {
     error("Not TS packet: 0x%02X", bufP[0]);
     return;
     }

  // Found TS packet
  int pid = ts_pid(bufP);
  int xpid = (bufP[1] << 8 | bufP[2]);

  // Check if payload available
  uint8_t count = payload(bufP);
  if (count == 0)
     return;

  if (xpid & 0x4000) {
     // Stream start (Payload Unit Start Indicator)
     uchar *d = (uint8_t*)bufP;
     d += 4;
     // pointer to payload
     if (bufP[3] & 0x20)
        d += d[0] + 1;
     // Skip adaption field
     if (bufP[3] & 0x10) {
        // Payload present
        if ((d[0] == 0) && (d[1] == 0) && (d[2] == 1)) {
           // PES packet start
           int sid = d[3];
           // Stream ID
           if ((sid >= 0xC0) && (sid <= 0xDF)) {
              if (pid < aPidM) {
                 debug("cPidScanner::%s(): found lower Apid: 0x%X instead of 0x%X", __FUNCTION__, pid, aPidM);
                 aPidM = pid;
                 numApidsM = 1;
                 }
              else if (pid == aPidM) {
                 ++numApidsM;
                 debug("cPidScanner::%s(): incrementing Apids, now at %d", __FUNCTION__, numApidsM);
                 }
              }
           else if ((sid >= 0xE0) && (sid <= 0xEF)) {
              if (pid < vPidM) {
                 debug("cPidScanner::%s(): found lower Vpid: 0x%X instead of 0x%X", __FUNCTION__, pid, vPidM);
                 vPidM = pid;
                 numVpidsM = 1;
                 }
              else if (pid == vPidM) {
                 ++numVpidsM;
                 debug("cPidScanner::%s(): incrementing Vpids, now at %d", __FUNCTION__, numVpidsM);
                 }
              }
           }
        if (((numVpidsM >= PIDSCANNER_VPID_COUNT) && (numApidsM >= PIDSCANNER_APID_COUNT)) ||
            (abs(numApidsM - numVpidsM) >= PIDSCANNER_PID_DELTA_COUNT)) {
           // Lock channels for pid updates
           if (!Channels.Lock(true, 10)) {
              timeoutM.Set(PIDSCANNER_TIMEOUT_IN_MS);
              return;
              }
           cChannel *IptvChannel = Channels.GetByChannelID(channelIdM);
           if (IptvChannel) {
              int Apids[MAXAPIDS + 1] = { 0 }; // these lists are zero-terminated
              int Atypes[MAXAPIDS + 1] = { 0 };
              int Dpids[MAXDPIDS + 1] = { 0 };
              int Dtypes[MAXDPIDS + 1] = { 0 };
              int Spids[MAXSPIDS + 1] = { 0 };
              char ALangs[MAXAPIDS][MAXLANGCODE2] = { "" };
              char DLangs[MAXDPIDS][MAXLANGCODE2] = { "" };
              char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
              int Vtype = IptvChannel->Vtype();
              int Ppid = IptvChannel->Ppid();
              int Tpid = IptvChannel->Tpid();
              bool foundApid = false;
              if (numVpidsM < PIDSCANNER_VPID_COUNT)
                 vPidM = 0; // No detected video pid
              else if (numApidsM < PIDSCANNER_APID_COUNT)
                 aPidM = 0; // No detected audio pid
              for (unsigned int i = 0; i < MAXAPIDS; ++i) {
                  Apids[i] = IptvChannel->Apid(i);
                  Atypes[i] = IptvChannel->Atype(i);
                  if (Apids[i] && (Apids[i] == aPidM))
                     foundApid = true;
                  }
              if (!foundApid) {
                 Apids[0] = aPidM;
                 Atypes[0] = 4;
                 }
              for (unsigned int i = 0; i < MAXDPIDS; ++i) {
                  Dpids[i] = IptvChannel->Dpid(i);
                  Dtypes[i] = IptvChannel->Dtype(i);
                  }
              for (unsigned int i = 0; i < MAXSPIDS; ++i)
                  Spids[i] = IptvChannel->Spid(i);
              debug("cPidScanner::%s(): vpid=0x%04X, apid=0x%04X", __FUNCTION__, vPidM, aPidM);
              IptvChannel->SetPids(vPidM, Ppid, Vtype, Apids, Atypes, ALangs, Dpids, Dtypes, DLangs, Spids, SLangs, Tpid);
              }
           Channels.Unlock();
           processM = false;
           }
        }
     }
}
