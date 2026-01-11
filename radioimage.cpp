/*
 * radioimage.c - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <cstdlib>
#include <vdr/tools.h>
#include <vdr/device.h>
#include "radioimage.h"
#include "config.h"

struct video_still_picture {
    char *iFrame;        /* pointer to a single iframe in memory */
    __s32 size;
};

// extern cRadioImage *RadioImage;
cRadioImage *RadioImage;

// --- cRadioImage -------------------------------------------------------

cRadioImage::cRadioImage() : cThread("radioimage") {
    imagepath = nullptr;
    imageShown = false;
    RadioImage = this;
}

cRadioImage::~cRadioImage() {
    if (Running()) {
        Stop();
    }

    free(imagepath);
}

void cRadioImage::Init() {
    RadioImage->Start();
}

void cRadioImage::Exit() {
    if (RadioImage != nullptr) {
        RadioImage->Stop();
        DELETENULL(RadioImage);
    }
}

void cRadioImage::Stop() {
    Cancel(2);
}

void cRadioImage::Action() {
    int count = 0;
    while (Running()) {
        cCondWait::SleepMs(20);
        count++;
        if (count % 15 == 0) {
            count = 0;
            if (imagepath && !imageShown) {
                imageShown = true;
                Show(imagepath);
            }
        }
    }
}

void cRadioImage::Show(const char *file) {
    uchar *buffer;
    int fd;
    struct stat st{};
    struct video_still_picture sp{};

    if (IptvConfig.GetStillPicture() > 1) // use with xineliboutput -> no stillpicture, but sound
        return;

    if ((fd = open(file, O_RDONLY)) >= 0) {
        fstat(fd, &st);
        sp.iFrame = (char *) malloc(st.st_size);
        if (sp.iFrame) {
            sp.size = st.st_size;
            if (read(fd, sp.iFrame, sp.size) > 0) {
                buffer = (uchar *) sp.iFrame;
                if (IptvConfig.GetStillPicture() > 0) {
                    send_still_picture(buffer, sp.size);
                } else {
                    for (int i = 1; i <= 25; i++)
                        send_pes_packet(buffer, sp.size, i);
                }
            }
            free(sp.iFrame);
        }
        close(fd);
    } else {
        printf("Unable to open file: %s\n", file);
    }
}

void cRadioImage::send_still_picture(unsigned char *payload, int payload_size)
{
    static unsigned char pes_packet[1024 * 1024];   // VDR's MAXFRAMESIZE is also KILOBYTE(1024)

    // PES start code
    pes_packet[0] = 0x00;
    pes_packet[1] = 0x00;
    pes_packet[2] = 0x01;
    // PES stream id
    pes_packet[3] = 0xE0;
    // PES payload size
    pes_packet[4] = 0x00;    // setting it to zero is allowed for video streams
    pes_packet[5] = 0x00;    // two bytes can only handle a size of 65.536
    // MPEG-2 PES header
    pes_packet[6] = 0x81;
    pes_packet[7] = 0x80;    // use pts
    pes_packet[8] = 0x05;    // pts length

    // presentation timestamp
    int timestamp = 30; // faked random timestamp, must be ignored in the output plugin

    int x = (0x02 << 4) | (((timestamp >> 30) & 0x07) << 1) | 1;
    pes_packet[9] = x;

    x = ((((timestamp >> 15) & 0x7FFF) << 1) | 1);
    pes_packet[10] = x >> 8;
    pes_packet[11] = x & 0xFF;

    x = ((((timestamp) & 0x7FFF) << 1) | 1);
    pes_packet[12] = x >> 8;
    pes_packet[13] = x & 0xFF;

    // copy the payload itself
    int pes_header_size = 6 + 3 + 5;
    memcpy(&pes_packet[pes_header_size], payload, payload_size);

    cDevice::PrimaryDevice()->StillPicture(pes_packet, pes_header_size + payload_size);
}

void cRadioImage::send_pes_packet(unsigned char *data, int len, int timestamp) {
#define PES_MAX_SIZE 2048
    int ptslen = timestamp ? 5 : 0;
    static unsigned char pes_header[PES_MAX_SIZE];
    pes_header[0] = 0;
    pes_header[1] = 0;
    pes_header[2] = 1;
    pes_header[3] = 0xe0;
    // MPEG-2 PES header
    pes_header[6] = 0x81;
    pes_header[7] = ptslen ? 0x80 : 0x0;
    pes_header[8] = ptslen;

    if (ptslen) {
        int x;
        // presentation time stamp:
        x = (0x02 << 4) | (((timestamp >> 30) & 0x07) << 1) | 1;
        pes_header[9] = x;
        x = ((((timestamp >> 15) & 0x7fff) << 1) | 1);
        pes_header[10] = x >> 8;
        pes_header[11] = x & 255;
        x = ((((timestamp) & 0x7fff) << 1) | 1);
        pes_header[12] = x >> 8;
        pes_header[13] = x & 255;
    }

    while (len > 0) {
        int payload_size = len;
        if (6 + 3 + ptslen + payload_size > PES_MAX_SIZE) {
            payload_size = PES_MAX_SIZE - (6 + 3 + ptslen);
        }
        pes_header[4] = (3 + ptslen + payload_size) >> 8;
        pes_header[5] = (3 + ptslen + payload_size) & 255;

        memcpy(&pes_header[6 + 3 + ptslen], data, payload_size);
        cDevice::PrimaryDevice()->PlayPes(pes_header, 6 + 3 + ptslen + payload_size);
        len -= payload_size;
        data += payload_size;
    }
}

void cRadioImage::SetBackgroundImage(const char *Image) {
    free(imagepath);
    imagepath = nullptr;

    if (Image) {
        imageShown = false;
        asprintf(&imagepath, "%s", Image);
    }
}
