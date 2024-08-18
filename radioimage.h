/*
 * radioimage.h - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <vdr/thread.h>

// Separate thread for showing RadioImages
class cRadioImage : public cThread {
private:
    char *imagepath;
    bool imageShown;
    void Show(const char *file);
    void send_pes_packet(unsigned char *data, int len, int timestamp);

protected:
    void Action() override;
    void Stop();

public:
    cRadioImage();
    ~cRadioImage() override;
    static void Init();
    static void Exit();
    void SetBackgroundImage(const char *Image);
};
