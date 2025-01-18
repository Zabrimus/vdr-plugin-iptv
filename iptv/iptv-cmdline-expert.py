#!/usr/bin/python3

import sys
import json

def readJson(str):
    return json.loads(str)

def prepareFFmpegVideo(json_object):
    # begin command line
    result = "ffmpeg -hide_banner -re -y" + \
             " -thread_queue_size " + str(json_object["threads"]) + \
             " -i \"" + json_object["url"] + "\""

    # add audio urls if they exists
    if 'audiourl' in json_object:
        for aurl in json_object['audiourl']:
            result += " -thread_queue_size " + str(json_object["threads"]) + \
                      " -i \"" + aurl + "\""

    # copy video
    result += " -c:v copy"

    # copy audio
    result += " -c:a copy"

    # copy subtitles
    # result += " -c:s copy"

    # map video/audio stream
    if 'audiourl' in json_object:
        i = 1
        result += " -map 0:v"
        for aurl in json_object['audiourl']:
            result += " -map " + str(i) + ":a"
            i += 1

    # add video pid
    result += " -streamid 0:" + str(json_object["vpid"])

    # add audio pid
    i = 1
    for apid in json_object['apid']:
        result += " -streamid " + str(i) + ":" + str(apid)
        i += 1

    # add all other parameters
    result += " -f mpegts"
    result += " -mpegts_transport_stream_id " + str(json_object["tpid"])
    result += " -mpegts_pmt_start_pid 4096"
    result += " -mpegts_service_id " + str(json_object["spid"])
    result += " -mpegts_original_network_id " + str(json_object["nid"])
    result += " -mpegts_flags system_b"
    # result += " -mpegts_flags nit"
    result += " -metadata service_name=\"" + json_object["channelName"] + "\""

    # print ts stream to stdout
    result += " pipe:1"

    return result

def prepareFFmpegAudio(json_object):
    # begin command line
    result = "ffmpeg -hide_banner -re -y" + \
             " -thread_queue_size " + str(json_object["threads"]) + \
             " -i \"" + json_object["url"] + "\""

    # copy audio
    result += " -c:a copy"

    # add audio pid
    i = 0
    for apid in json_object['apid']:
        result += " -streamid " + str(i) + ":" + str(apid)
        i += 1

    # add all other parameters
    result += " -f mpegts"
    result += " -mpegts_transport_stream_id " + str(json_object["tpid"])
    result += " -mpegts_pmt_start_pid 4096"
    result += " -mpegts_service_id " + str(json_object["spid"])
    result += " -mpegts_original_network_id " + str(json_object["nid"])
    result += " -mpegts_flags system_b"
    # result += " -mpegts_flags nit"
    result += " -metadata service_name=\"" + json_object["channelName"] + "\""

    # print ts stream to stdout
    result += " pipe:1"

    return result

def prepareVlcVideo(json_object):
    result = "vlc" + " -I" + " dummy" + " -v" + \
             " --network-caching=4000" + " --live-caching" + " 2000" + \
             " --http-reconnect" + " --http-user-agent=Mozilla/5.0" + \
             " --adaptive-logic" + " highest"

    result += " \"" + json_object["url"] + "\""
    result += " --sout"
    result += " \"#standard{access=file,mux=ts{use-key-frames,pid-video=" + \
              str(json_object["vpid"]) + ",pid-audio=" + str(json_object["apid"][0]) + ",pid-spu=4096,tsid="+ str(json_object["tpid"]) + "},dst=-}\""

    return result

def prepareVlcAudio(json_object):
    result = "vlc" + " -I" + " dummy" + " --verbose=2" + " --no-stats"
    result += " \"" + json_object["url"] + "\""
    result += " --sout"
    result += " \"#transcode{acodec=mpga,ab=320}:standard{access=file,mux=ts{pid-audio=" + str(json_object["apid"][0]) + ",pid-spu=4096,tsid=" + str(json_object["tpid"]) + "},dst=-}\""

    return result


if __name__ == '__main__':
    json_object = readJson(sys.argv[2])
    # print(json.dumps(json_object, indent=4))

    # Copy of the FFmpeg/vlc calls in vdr-plugin-iptv
    # Do whatever you want to do. E.g change frame rate, reencode audio, ...
    # It's your your choice to change the FFmpeg or vlc command line
    if sys.argv[1] == '1':
        print(prepareFFmpegVideo(json_object))
    else:
        print(prepareVlcAudio(json_object))




