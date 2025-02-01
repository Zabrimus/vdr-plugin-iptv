/*
 * protocoludp.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/device.h>

#include "config.h"
#include "log.h"
#include "socket.h"
#include "protocoltcp.h"

cIptvProtocolTcp::cIptvProtocolTcp()
    : streamAddrM(strdup("")),
      streamPortM(0) {
    debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolTcp::~cIptvProtocolTcp() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Drop the multicast group and close the socket
    cIptvProtocolTcp::Close();

    // Free allocated memory
    free(streamAddrM);
}

bool cIptvProtocolTcp::Open() {
    debug1("%s streamAddr='%s'", __PRETTY_FUNCTION__, streamAddrM);

    OpenSocket(streamPortM, streamAddrM);

    return true;
}

bool cIptvProtocolTcp::Close() {
    debug1("%s streamAddr='%s'", __PRETTY_FUNCTION__, streamAddrM);

    // Close the socket
    CloseSocket();

    return true;
}

int cIptvProtocolTcp::Read(unsigned char *bufferAddrP, unsigned int bufferLenP) {
    return cIptvTcpServerSocket::Read(bufferAddrP, bufferLenP);
}

bool cIptvProtocolTcp::SetSource(SourceParameter parameter) {
    debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, parameter.locationP, parameter.parameterP, parameter.indexP);

    if (!isempty(parameter.locationP)) {
        // Update stream address and port
        streamAddrM = strcpyrealloc(streamAddrM, parameter.locationP);
        streamPortM = parameter.parameterP;
    }

    OpenSocket(streamPortM);

    return true;
}

bool cIptvProtocolTcp::SetPid(int pidP, int typeP, bool onP) {
    debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);

    return true;
}

cString cIptvProtocolTcp::GetInformation() {
    debug16("%s", __PRETTY_FUNCTION__);

    return cString::sprintf("tcp://%s:%d", streamAddrM, streamPortM);
}
