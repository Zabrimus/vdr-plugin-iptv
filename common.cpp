/*
 * common.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <cctype>
#include <memory>
#include <string>
#include <vdr/tools.h>
#include <vdr/channels.h>
#include <mutex>
#include "common.h"

std::mutex all404ChannelMutex;
std::set<int> all404Channels;

uint16_t ts_pid(const uint8_t *bufP) {
    return (uint16_t) (((bufP[1] & 0x1f) << 8) + bufP[2]);
}

uint8_t payload(const uint8_t *bufP) {
    if (!(bufP[3] & 0x10)) {// no payload?
        return 0;
    }

    if (bufP[3] & 0x20) {  // adaptation field?
        if (bufP[4] > 183) { // corrupted data?
            return 0;
        } else {
            return (uint8_t) ((184 - 1) - bufP[4]);
        }
    }

    return 184;
}

const char *id_pid(const u_short pidP) {
    for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i) {
        if (pidP==section_filter_table[i].pid) {
            return section_filter_table[i].tag;
        }
    }
    return "---";
}

int select_single_desc(int descriptorP, const int usecsP, const bool selectWriteP) {
    // Wait for data
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = usecsP;
    // Use select
    fd_set infd;
    fd_set outfd;
    fd_set errfd;
    FD_ZERO(&infd);
    FD_ZERO(&outfd);
    FD_ZERO(&errfd);
    FD_SET(descriptorP, &errfd);

    if (selectWriteP) {
        FD_SET(descriptorP, &outfd);
    } else {
        FD_SET(descriptorP, &infd);
    }

    int retval = select(descriptorP + 1, &infd, &outfd, &errfd, &tv);

    // Check if error
    ERROR_IF_RET(retval < 0, "select()", return retval);
    return retval;
}

cString ChangeCase(const cString &strP, bool upperP) {
    cString res(strP);
    char *p = (char *) *res;

    while (p && *p) {
        *p = upperP ? toupper(*p) : tolower(*p);
        ++p;
    }
    return res;
}

const section_filter_table_type section_filter_table[SECTION_FILTER_TABLE_SIZE] =
    {
        /* description                        tag    pid   tid   mask */
        {trNOOP("PAT (0x00)"), "PAT", 0x00, 0x00, 0xFF},
        {trNOOP("NIT (0x40)"), "NIT", 0x10, 0x40, 0xFF},
        {trNOOP("SDT (0x42)"), "SDT", 0x11, 0x42, 0xFF},
        {trNOOP("EIT (0x4E/0x4F/0x5X/0x6X)"), "EIT", 0x12, 0x40, 0xC0},
        {trNOOP("TDT (0x70)"), "TDT", 0x14, 0x70, 0xFF},
    };

std::string ReplaceAll(std::string str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;

    while ((start_pos = str.find(from, start_pos))!=std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

void printBacktrace() {
    cStringList stringList;

    cBackTrace::BackTrace(stringList, 0, false);
    esyslog("[iptv] Backtrace size: %d", stringList.Size());

    for (int i = 0; i < stringList.Size(); ++i) {
        esyslog("[iptv] ==> %s", stringList[i]);
    }

    esyslog("[iptv] ==> Caller: %s", *cBackTrace::GetCaller());
}

void mark404Channel(int channelId) {
    /*
    std::lock_guard<std::mutex> guard(all404ChannelMutex);
    all404Channels.emplace(channelId);
    */
}

void rename404Channels() {
    /*
    if (all404Channels.empty()) {
        return;
    }

    std::lock_guard<std::mutex> guard(all404ChannelMutex);

    for (auto c : all404Channels) {
        {
            LOCK_CHANNELS_WRITE;
            cChannel *channel = Channels->GetByNumber(c);

            if (channel) {
                if (!endswith(channel->Name(), CHANNELMARK404)) {
                    channel->SetName(cString::sprintf("%s %s", channel->Name(), CHANNELMARK404),
                                     channel->ShortName(),
                                     cString::sprintf("%s %s", CHANNELMARK404, channel->Provider()));
                }
            }
        }
    }
    */
}

