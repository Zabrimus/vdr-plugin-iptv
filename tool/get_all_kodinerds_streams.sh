#!/bin/bash

# clone the repository
git clone --depth 1 https://github.com/jnk22/kodinerds-iptv

# convert all kodi files
./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_tv_main.m3u kodi_tv_main.cfg kodi_tv_main.channels.conf 1 0
./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_tv_local.m3u kodi_tv_local.cfg kodi_tv_local.channels.conf 2 0
./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_tv_regional.m3u kodi_tv_regional.cfg kodi_tv_regional.channels.conf 3 0
./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_tv_atch.m3u kodi_tv_atch.cfg kodi_tv_atch.channels.conf 4 0
./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_tv_international.m3u kodi_tv_international.cfg kodi_tv_international.channels.conf 5 0
./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_tv_usuk.m3u kodi_tv_usuk.cfg kodi_tv_usuk.channels.conf 6 0
./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_tv_extra.m3u kodi_tv_extra.cfg kodi_tv_extra.channels.conf 7 0
./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_tv_shop.m3u kodi_tv_shop.cfg kodi_tv_shop.channels.conf 8 0

#./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_radio_de.m3u kodi_radio_de.cfg kodi_radio_de.channels.conf 9 1
#./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_radio_ro.m3u kodi_radio_ro.cfg kodi_radio_ro.channels.conf 10 1
#./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_radio_at.m3u kodi_radio_at.cfg kodi_radio_at.channels.conf 11 1
#./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_radio_fr.m3u kodi_radio_fr.cfg kodi_radio_fr.channels.conf 12 1
#./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_radio_nl.m3u kodi_radio_nl.cfg kodi_radio_nl.channels.conf 13 1
#./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_radio_uk.m3u kodi_radio_uk.cfg kodi_radio_uk.channels.conf 14 1
#./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_radio_ch.m3u kodi_radio_ch.cfg kodi_radio_ch.channels.conf 15 1
#./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_radio_it.m3u kodi_radio_it.cfg kodi_radio_it.channels.conf 16 1
#./convm3u_channel kodinerds-iptv/iptv/kodi/kodi_radio_pl.m3u kodi_radio_pl.cfg kodi_radio_pl.channels.conf 16 1
