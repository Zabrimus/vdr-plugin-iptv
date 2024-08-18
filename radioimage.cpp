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
    while (Running()) {
        cCondWait::SleepMs(333);
        if (imagepath && !imageShown) {
            imageShown = true;
            Show(imagepath);
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
                    cDevice::PrimaryDevice()->StillPicture(buffer, sp.size);
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
