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
URL="http://dash.akamaized.net/dash264/TestCases/3b/fraunhofer/aac-lc_stereo_with_video/ElephantsDream/elephants_dream_480p_aaclc_stereo_sidx.mpd"

/usr/local/bin/dash2ts -p ${PORT} -u ${URL}w
