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

CFGFILE="$(dirname $(realpath $0))/m3u-list-main.cfg"

# Channels.conf parameter (A=xxx)
PARAMETER=${1}

# Iptv plugin listens this port
PORT=${2}

# find URL to stream (configured in file m3u-list-main.cfg)
URL=$(grep "^${PARAMETER}:" ${CFGFILE} | sed -e "s/^${PARAMETER}://")

# use parameter A to select either ffmpeg or netcat
/storage/.kodi/addons/yt-dlp/bin/yt-dlp.sh  -t mp4 --audio-multistreams --hls-use-mpegts ${URL} -o - | /usr/bin/netcat 127.0.0.1 ${PORT}
