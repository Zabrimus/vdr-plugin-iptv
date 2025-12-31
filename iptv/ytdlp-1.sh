#!/bin/sh
#
# iptvstream.sh can be used by the VDR iptv plugin to transcode external
# sources
#
# (C) 2007 Rolf Ahrenberg, Antti Seppälä
#
# iptvstream.sh is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this package; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
# MA 02110-1301, USA.

if [ $# -ne 2 ]; then
    logger "$0: error: Invalid parameter count '$#' $*"
    exit 1
fi

# Channels.conf parameter (A=xxx)
PARAMETER=${1}

# Iptv plugin listens this port
PORT=${2}

# URL to stream
URL="https://daserste-live.ard-mcdn.de/daserste/live/hls/de/master.m3u8"

# use parameter A to select either ffmpeg or netcat
if [ "${A}" = 1 ]; then
    /usr/local/bin/yt-dlp_linux -t mp4 --audio-multistreams --hls-use-mpegts ${URL} -o - | ffmpeg -i - -codec copy -f mpegts tcp://127.0.0.1:${PORT}
else
    /usr/local/bin/yt-dlp_linux -t mp4 --audio-multistreams --hls-use-mpegts ${URL} -o - | nc 127.0.0.1 ${PORT}
fi