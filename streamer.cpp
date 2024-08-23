/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/plugin.h>
#include "common.h"
#include "log.h"
#include "streamer.h"

cIptvStreamer::cIptvStreamer(cIptvDeviceIf &deviceP, unsigned int packetLenP)
    : cThread("IPTV streamer"),
      sleepM(),
      deviceM(&deviceP),
      packetBufferLenM(packetLenP),
      protocolM(nullptr),
      radioImage(nullptr) {

    debug1("%s (, %d)", __PRETTY_FUNCTION__, packetBufferLenM);

    // Allocate packet buffer
    packetBufferM = MALLOC(unsigned char, packetBufferLenM);

    if (packetBufferM) {
        memset(packetBufferM, 0, packetBufferLenM);
    } else {
        error("MALLOC() failed for packet buffer");
    }
}

cIptvStreamer::~cIptvStreamer() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Close the protocol
    Close();
    protocolM = nullptr;

    // Free allocated memory
    free(packetBufferM);
}

void cIptvStreamer::Action() {
    debug1("%s() Entering", __PRETTY_FUNCTION__);

    // Increase priority
    //SetPriority(-1);

    // Do the thread loop
    while (packetBufferM && Running()) {
        int length = -1;

        unsigned int size = min(deviceM->CheckData(), packetBufferLenM);
        if (protocolM && (size > 0)) {
            length = protocolM->Read(packetBufferM, size);
        }

        if (length > 0) {
            AddStreamerStatistic(length);
            deviceM->WriteData(packetBufferM, length);
        } else {
            sleepM.Wait(10); // to avoid busy loop and reduce cpu load
        }
    }

    debug1("%s Exiting", __PRETTY_FUNCTION__);
}

bool cIptvStreamer::Open() {
    // printf("Vor Open Mutex\n");
    // std::lock_guard<std::mutex> guard(streamerMutex);
    // printf("Nach Open Mutex\n");

    debug1("%s", __PRETTY_FUNCTION__);

    // Open the protocol
    if (protocolM && !protocolM->Open())
        return false;

    // check if this is a radio channel (vpid == 0)
    bool isRadio = false;
    {
        LOCK_CHANNELS_READ;
        auto c = Channels->GetByNumber(channelNumber);
        if (c && c->Vpid() == 0) {
            isRadio = true;
        }
    }

    // check if the radio plugin exists
    bool hasRadioPlugin = false;
    auto p = cPluginManager::GetPlugin("radio");
    if (p) {
        hasRadioPlugin = true;
    }

    // start RadioImage if this is a radio channel and radio plugin does not exists
    if (isRadio && !hasRadioPlugin) {
        if (radioImage != nullptr) {
            radioImage->Exit();
        }

        radioImage = new cRadioImage();

        cString img = cString::sprintf("%s/%s", IptvConfig.GetResourceDirectory(), "radio.mpg");
        radioImage->SetBackgroundImage(*img);
        radioImage->Start();
    }

    // Start thread
    Start();

    return true;
}

bool cIptvStreamer::Close() {
    // printf("Vor Close Mutex\n");
    // std::lock_guard<std::mutex> guard(streamerMutex);
    // printf("Nach close Mutex\n");

    debug1("%s", __PRETTY_FUNCTION__);

    // Stop thread
    sleepM.Signal();

    if (Running()) {
        Cancel(3);
    }

    // Close the protocol
    if (protocolM) {
        protocolM->Close();
    }

    return true;
}

bool cIptvStreamer::SetSource(cIptvProtocolIf *protocolP, SourceParameter parameter) {
    // printf("Vor SetSource Mutex\n");
    // std::lock_guard<std::mutex> guard(streamerMutex);
    // printf("Nach SetSource Mutex\n");

    debug1("%s (%s, %d, %d, ChannelNumber: %d)", __PRETTY_FUNCTION__, parameter.locationP, parameter.parameterP, parameter.indexP, parameter.channelNumber);

    if (!isempty(parameter.locationP)) {
        channelNumber = parameter.channelNumber;

        // Update protocol and set location and parameter; Close the existing one if changed
        if (protocolM!=protocolP) {
            if (protocolM) {
                protocolM->Close();
            }

            protocolM = protocolP;
            if (protocolM) {
                if (!protocolM->SetSource(parameter)) {
                    return false;
                }

                protocolM->Open();
            }

        } else if (protocolM) {
            if (!protocolM->SetSource(parameter)) {
                return false;
            }
        }
    }

    return true;
}

bool cIptvStreamer::SetPid(int pidP, int typeP, bool onP) {
    debug9("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);

    if (protocolM) {
        return protocolM->SetPid(pidP, typeP, onP);
    }

    return true;
}

cString cIptvStreamer::GetInformation() {
    debug16("%s", __PRETTY_FUNCTION__);

    cString s;
    if (protocolM) {
        s = protocolM->GetInformation();
    }

    return s;
}
