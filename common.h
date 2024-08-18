/*
 * common.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <string>
#include <vdr/tools.h>
#include <vdr/config.h>
#include <vdr/i18n.h>
#include <set>

#define ELEMENTS(x)                     (sizeof(x) / sizeof(x[0]))
#define IPTV_BUFFER_SIZE                KILOBYTE(2048)
#define IPTV_DVR_FILENAME               "/tmp/vdr-iptv%d.dvr"
#define IPTV_SOURCE_CHARACTER           'I'

#define IPTV_DEVICE_INFO_ALL            0
#define IPTV_DEVICE_INFO_GENERAL        1
#define IPTV_DEVICE_INFO_PIDS           2
#define IPTV_DEVICE_INFO_FILTERS        3
#define IPTV_DEVICE_INFO_PROTOCOL       4
#define IPTV_DEVICE_INFO_BITRATE        5
#define IPTV_STATS_ACTIVE_PIDS_COUNT    10
#define IPTV_STATS_ACTIVE_FILTERS_COUNT 10
#define SECTION_FILTER_TABLE_SIZE       5

#define ERROR_IF_FUNC(exp, errstr, func, ret)                  \
  do {                                                         \
     if (exp) {                                                \
        char tmp[64];                                          \
        esyslog("[%s,%d]: " errstr ": %s", __FILE__, __LINE__, \
                strerror_r(errno, tmp, sizeof(tmp)));          \
        func;                                                  \
        ret;                                                   \
        }                                                      \
  } while (0)

#define ERROR_IF_RET(exp, errstr, ret) ERROR_IF_FUNC(exp, errstr, ,ret);
#define ERROR_IF(exp, errstr) ERROR_IF_FUNC(exp, errstr, , );

#define DELETE_POINTER(ptr)       \
  do {                            \
     if (ptr) {                   \
        typeof(*ptr) *tmp = ptr;  \
        ptr = NULL;               \
        delete(tmp);              \
        }                         \
  } while (0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

uint16_t ts_pid(const uint8_t *bufP);
uint8_t payload(const uint8_t *bufP);
const char *id_pid(const u_short pidP);
int select_single_desc(int descriptorP, const int usecsP, const bool selectWriteP);
cString ChangeCase(const cString &strP, bool upperP);
std::string ReplaceAll(std::string str, const std::string &from, const std::string &to);

inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
      return !std::isspace(ch);
    }));
}

inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
      return !std::isspace(ch);
    }).base(), s.end());
}

inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

void printBacktrace();


#define CHANNELMARK404 "- 404"
extern std::set<int> all404Channels;
void mark404Channel(int channelId);
void rename404Channels();

struct section_filter_table_type {
  const char *description;
  const char *tag;
  u_short pid;
  u_char tid;
  u_char mask;
};

extern const section_filter_table_type section_filter_table[SECTION_FILTER_TABLE_SIZE];

extern const char VERSION[];

