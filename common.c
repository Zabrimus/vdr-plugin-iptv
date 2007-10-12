/*
 * common.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: common.c,v 1.3 2007/10/10 19:41:10 rahrenbe Exp $
 */

#include <vdr/i18n.h>
#include <vdr/tools.h>
#include "common.h"

uint16_t ts_pid(const uint8_t *buf)
{
  return ((buf[1] & 0x1f) << 8) + buf[2];
}

uint8_t payload(const uint8_t *tsp)
{
  if (!(tsp[3] & 0x10))        // no payload?
     return 0;

  if (tsp[3] & 0x20) { // adaptation field?
     if (tsp[4] > 183) // corrupted data?
        return 0;
     else
        return 184 - 1 - tsp[4];
     }

  return 184;
}

const char *id_pid(const u_short Pid)
{
  for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i) {
      if (Pid == section_filter_table[i].pid)
         return section_filter_table[i].tag;
      }
  return "---";
}

const section_filter_table_type section_filter_table[SECTION_FILTER_TABLE_SIZE] =
{
  /* description              tag    pid   tid   mask */
  {trNOOP("PAT (0x00)"),      "PAT", 0x00, 0x00, 0xFF},
  {trNOOP("NIT (0x40)"),      "NIT", 0x10, 0x40, 0xFF},
  {trNOOP("SDT (0x42)"),      "SDT", 0x11, 0x42, 0xFF},
  {trNOOP("EIT (0x4E/0x4F)"), "EIT", 0x12, 0x4E, 0xFE},
  {trNOOP("EIT (0x5X)"),      "EIT", 0x12, 0x50, 0xF0},
  {trNOOP("EIT (0x6X)"),      "EIT", 0x12, 0x60, 0xF0},
  {trNOOP("TDT (0x70)"),      "TDT", 0x14, 0x70, 0xFF},
};
