#pragma once

#include <iostream>
#include <fstream>

#undef error
#include "uri_parser.h"

#undef error
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

struct media {
    std::string type;
    std::string language;
    std::string name;
    std::string groupId;
    std::string uri;
};

typedef struct {
    int width;
    int height;
    std::string url;
    std::vector<media> audio;

    std::string channelName;
    int vpid;
    int spid;
    int tpid;
    int nid;
    std::vector<int> apids;
} m3u_stream;

class M3u8Handler {
public:
    explicit M3u8Handler();
    static m3u_stream parseM3u(const std::string &uri, int useYtdlp);
    static m3u_stream parseYT(const std::string &webUri);

    static void printStream(const m3u_stream& stream);

private:
    static bool startsWith(const std::string &str, const std::string &prefix);
    static std::vector<std::string> split(const std::string &s, char delim);
};