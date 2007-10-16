/*
 * protocolext.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolext.c,v 1.3 2007/10/16 22:13:44 rahrenbe Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>
#include <vdr/plugin.h>

#include "common.h"
#include "config.h"
#include "protocolext.h"

cIptvProtocolExt::cIptvProtocolExt()
: listenPort(4321),
  socketDesc(-1),
  readBufferLen(TS_SIZE * IptvConfig.GetReadBufferTsCount()),
  isActive(false)
{
  debug("cIptvProtocolExt::cIptvProtocolExt()\n");
  streamAddr = strdup("");
  listenAddr = strdup("127.0.0.1");
  // Allocate receive buffer
  readBuffer = MALLOC(unsigned char, readBufferLen);
  if (!readBuffer)
     error("ERROR: MALLOC() failed in ProtocolExt()");
}

cIptvProtocolExt::~cIptvProtocolExt()
{
  debug("cIptvProtocolExt::~cIptvProtocolExt()\n");
  // Drop the multicast group and close the socket
  Close();
  // Free allocated memory
  free(streamAddr);
  free(listenAddr);
  free(readBuffer);
}

bool cIptvProtocolExt::OpenSocket(void)
{
  debug("cIptvProtocolExt::OpenSocket()\n");
  // Bind to the socket if it is not active already
  if (socketDesc < 0) {
     int yes = 1;     
     // Create socket
     socketDesc = socket(PF_INET, SOCK_DGRAM, 0);
     if (socketDesc < 0) {
        char tmp[64];
        error("ERROR: socket(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return false;
        }
     // Make it use non-blocking I/O to avoid stuck read calls
     if (fcntl(socketDesc, F_SETFL, O_NONBLOCK)) {
        char tmp[64];
        error("ERROR: fcntl(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }
     // Allow multiple sockets to use the same PORT number
     if (setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &yes,
		    sizeof(yes)) < 0) {
        char tmp[64];
        error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }
     // Bind socket
     memset(&sockAddr, '\0', sizeof(sockAddr));
     sockAddr.sin_family = AF_INET;
     sockAddr.sin_port = htons(listenPort);
     sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     int err = bind(socketDesc, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
     if (err < 0) {
        char tmp[64];
        error("ERROR: bind(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }
     }
  return true;
}

void cIptvProtocolExt::CloseSocket(void)
{
  debug("cIptvProtocolExt::CloseSocket()\n");
  // Check if socket exists
  if (socketDesc >= 0) {
     close(socketDesc);
     socketDesc = -1;
     }
}


int cIptvProtocolExt::Read(unsigned char* *BufferAddr)
{
  //debug("cIptvProtocolExt::Read()\n");
  socklen_t addrlen = sizeof(sockAddr);
  // Set argument point to read buffer
  *BufferAddr = readBuffer;
  // Wait for data
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  // Use select
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(socketDesc, &rfds);
  int retval = select(socketDesc + 1, &rfds, NULL, NULL, &tv);
  // Check if error
  if (retval < 0) {
     char tmp[64];
     error("ERROR: select(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     return -1;
     }
  // Check if data available
  else if (retval) {
     // Read data from socket
     int len = recvfrom(socketDesc, readBuffer, readBufferLen, MSG_DONTWAIT,
                        (struct sockaddr *)&sockAddr, &addrlen);
     if ((len > 0) && (readBuffer[0] == 0x47)) {
        // Set argument point to read buffer
        *BufferAddr = &readBuffer[0];
        return len;
        }
     else if (len > 3) {
        // http://www.networksorcery.com/enp/rfc/rfc2250.txt
        // version
        unsigned int v = (readBuffer[0] >> 6) & 0x03;
        // extension bit
        unsigned int x = (readBuffer[0] >> 4) & 0x01;
        // cscr count
        unsigned int cc = readBuffer[0] & 0x0F;
        // payload type
        unsigned int pt = readBuffer[1] & 0x7F;
        // header lenght
        unsigned int headerlen = (3 + cc) * sizeof(uint32_t);
        // check if extension
        if (x) {
           // extension header length
           unsigned int ehl = (((readBuffer[headerlen + 2] & 0xFF) << 8) | (readBuffer[headerlen + 3] & 0xFF));
           // update header length
           headerlen += (ehl + 1) * sizeof(uint32_t);
           }
        // Check that rtp is version 2, payload type is MPEG2 TS
        // and payload contains multiple of TS packet data
        if ((v == 2) && (pt == 33) && (((len - headerlen) % TS_SIZE) == 0)) {
           // Set argument point to payload in read buffer
           *BufferAddr = &readBuffer[headerlen];
           return (len - headerlen);
           }
        }
     }
  return 0;
}

bool cIptvProtocolExt::Open(void)
{
  debug("cIptvProtocolExt::Open(): streamAddr=%s\n", streamAddr);

  // Reject completely empty stream addresses
  if (!strlen(streamAddr))
      return false;

  // Create the listening socket
  OpenSocket();
  if (!isActive) {
    char* cmd = NULL;
    asprintf(&cmd, "%s start", streamAddr);
    int retval = SystemExec(cmd);
    free(cmd);
    if (!retval)
      isActive = true;
  }
  return isActive;
}

bool cIptvProtocolExt::Close(void)
{
  debug("cIptvProtocolExt::Close(): streamAddr=%s\n", streamAddr);
  // Close the socket
  CloseSocket();
  if (isActive) {
    char* cmd = NULL;
    asprintf(&cmd, "%s stop", streamAddr);
    int retval = SystemExec(cmd);
    free(cmd);
    if (!retval)
      isActive = false;
  }

  return !isActive;
}

bool cIptvProtocolExt::Set(const char* Address, const int Port)
{
  debug("cIptvProtocolExt::Set(): %s:%d\n", Address, Port);
  if (!isempty(Address)) {
    // Update stream address and port
    streamAddr = strcpyrealloc(streamAddr, Address);
    listenPort = Port;
    }
  return true;
}

cString cIptvProtocolExt::GetInformation(void)
{
  //debug("cIptvProtocolExt::GetInformation()");
  return cString::sprintf("ext://%s:%d", streamAddr, listenPort);
}