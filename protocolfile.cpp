/*
 * protocolfile.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/device.h>

#include "common.h"
#include "config.h"
#include "log.h"
#include "protocolfile.h"

cIptvProtocolFile::cIptvProtocolFile()
    : fileLocationM(strdup("")),
      fileDelayM(0),
      fileStreamM(nullptr),
      isActiveM(false) {
    debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolFile::~cIptvProtocolFile() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Drop open handles
    cIptvProtocolFile::Close();

    // Free allocated memory
    free(fileLocationM);
}

bool cIptvProtocolFile::OpenFile() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Check that stream address is valid
    if (!isActiveM && !isempty(fileLocationM)) {
        fileStreamM = fopen(fileLocationM, "rb");
        ERROR_IF_RET(!fileStreamM || ferror(fileStreamM), "fopen()", return false);
        // Update active flag
        isActiveM = true;
    }

    return true;
}

void cIptvProtocolFile::CloseFile() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Check that file stream is valid
    if (isActiveM && !isempty(fileLocationM)) {
        fclose(fileStreamM);
        // Update active flag
        isActiveM = false;
    }
}

int cIptvProtocolFile::Read(unsigned char *bufferAddrP, unsigned int bufferLenP) {
    debug16("%s (, %u)", __PRETTY_FUNCTION__, bufferLenP);

    // Check errors
    if (!fileStreamM || ferror(fileStreamM)) {
        debug1("%s (, %d) Stream error", __PRETTY_FUNCTION__, bufferLenP);
        return -1;
    }

    // Rewind if EOF
    if (feof(fileStreamM)) {
        rewind(fileStreamM);
    }

    // Sleep before reading the file stream to prevent aggressive busy looping
    // and prevent transfer ringbuffer overflows
    if (fileDelayM) {
        cCondWait::SleepMs(fileDelayM);
    }

    // This check is to prevent a race condition where file may be switched off
    // during the sleep and buffers are disposed. Check here that the plugin is
    // still active before accessing the buffers
    if (isActiveM) {
        return (int) fread(bufferAddrP, sizeof(unsigned char), bufferLenP, fileStreamM);
    }

    return -1;
}

bool cIptvProtocolFile::Open() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Open the file stream
    OpenFile();

    return true;
}

bool cIptvProtocolFile::Close() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Close the file stream
    CloseFile();

    return true;
}

bool cIptvProtocolFile::SetSource(SourceParameter parameter) {
    debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, parameter.locationP, parameter.parameterP, parameter.indexP);

    if (!isempty(parameter.locationP)) {
        // Close the file stream
        CloseFile();

        // Update stream address and port
        fileLocationM = strcpyrealloc(fileLocationM, parameter.locationP);
        fileDelayM = parameter.parameterP;

        // Open the file for input
        OpenFile();
    }

    return true;
}

bool cIptvProtocolFile::SetPid(int pidP, int typeP, bool onP) {
    debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);

    return true;
}

cString cIptvProtocolFile::GetInformation() {
    debug16("%s", __PRETTY_FUNCTION__);

    return cString::sprintf("file://%s:%d", fileLocationM, fileDelayM);
}
