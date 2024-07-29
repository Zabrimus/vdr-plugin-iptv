#include "m3u8handler.h"
#include "log.h"
#include "config.h"
#include "process.hpp"

std::pair<std::string, std::string> splitUri(std::string uri) {
    auto yturi = uri::parse_uri(uri);
    std::string host = yturi.scheme + "://" + yturi.authority.host + (yturi.authority.port > 0 ? std::to_string(yturi.authority.port) : "");

    std::string query;
    for (auto a : yturi.query) {
        query.append(a.first).append("=").append(a.second).append("&");
    }

    std::string path = yturi.path + "?" + query;

    return std::make_pair(host, path);

}

M3u8Handler::M3u8Handler() {
}

bool M3u8Handler::startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

std::vector<std::string> M3u8Handler::split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}

m3u_stream M3u8Handler::parseM3u(const std::string &webUri, int useYtdlp) {
    std::string useUri;

    // if useYtdlp == 1 then use yt-dlp to get the real m3u8 URL
    // if useYtdlp == 2 then get the page and try to find the real m3u8 URL
    if (useYtdlp == 1) {
        std::vector<std::string> callStr {
            IptvConfig.GetYtdlpPath(), "--get-url", webUri
        };

        std::string newUri;
        auto handler = new TinyProcessLib::Process(callStr, "",
                              [&newUri](const char *bytes, size_t n) {
                                  std::string result = std::string(bytes, n);
                                  debug1("yt-dlp found URL %s\n", result.c_str());
                                  newUri = std::string(bytes, n);
                              },

                              [](const char *bytes, size_t n) {
                                    std::string msg = std::string(bytes, n);
                                    debug1("yt-dlp Error: %s\n", msg.c_str());
                              },

                              true
        );

        int exitStatus = handler->get_exit_status();
        if (exitStatus != 0) {
            debug1("yt-dlp throws an error, abort\n");
            m3u_stream result;
            result.width = result.height = 0;
            return result;
        }

        useUri = newUri;
    } else if (useYtdlp == 2) {
        auto yturi = splitUri(webUri);

        httplib::Client ytcli(yturi.first);
        ytcli.set_follow_location(true);
        auto ytres = ytcli.Get(yturi.second);

        m3u_stream ytresult;
        ytresult.width = ytresult.height = 0;

        if (ytres == nullptr) {
            error("Got no result for request %s\n", webUri.c_str());
            return ytresult;
        }

        if (ytres->status != 200) {
            debug1("Got HTTP result code %d\n", ytres->status);
            return ytresult;
        }

        auto m = ytres->body.find(".m3u");
        auto index = ytres->body.rfind("http", m);
        useUri = ytres->body.substr(index, m + 4 - index);
    } else {
        useUri = webUri;
    }

    auto uri = splitUri(useUri);

    httplib::Client cli(uri.first);
    cli.set_follow_location(true);
    auto res = cli.Get(uri.second);

    m3u_stream result;
    result.width = result.height = 0;

    if (res == nullptr) {
        return result;
    }

    if (res->status != 200) {
        return result;
    }

    std::string m3u = res->body;
    std::istringstream stream(m3u);
    std::string line;

    int maxW = 0, maxH = 0;
    std::string audioGroup;
    std::string m3uMax;

    std::string starterStream = "#EXT-X-STREAM-INF:";
    std::string starterMedia = "#EXT-X-MEDIA:";
    std::string starterIgnore = "#EXTINF:";

    while (std::getline(stream, line)) {
        if (startsWith(line, starterStream)) {
            auto splitted = split(line.substr(starterStream.length()), ',');

            for (const auto& t : splitted) {
                if (startsWith(t, "RESOLUTION=")) {
                    int w, h;
                    sscanf(t.c_str() + 11, "%dx%d", &w, &h);

                    debug2("Found Resolution: %dx%d\n", w, h);

                    if (w > maxW) {
                        maxW = w;
                        maxH = h;
                        std::getline(stream, m3uMax);

                        if (!startsWith(m3uMax, "http://") && !startsWith(m3uMax, "https://")) {
                            // this is a relative URL -> construct absolute URL
                            auto last = useUri.find_last_of('/');
                            m3uMax = useUri.substr(0, last+1).append(m3uMax);
                        }
                    }
                } else if (startsWith(t, "AUDIO=")) {
                    audioGroup = t.substr(7, t.length()-8);
                }
            }
        } else if (startsWith(line, starterMedia)) {
            auto splitted = split(line.substr(starterMedia.length()), ',');

            bool addThis = true;
            media m;

            for (const auto& t : splitted) {
                if (startsWith(t, "TYPE=")) {
                    m.type = t.substr(5);
                    if (m.type != "AUDIO") {
                        // no audio -> skip
                        addThis = false;
                        break;
                    }
                } else if (startsWith(t, "LANGUAGE=")) {
                    m.language = t.substr(10, t.length()-11);
                } else if (startsWith(t, "NAME=")) {
                    m.name = t.substr(6, t.length()-7);
                } else if (startsWith(t, "GROUP-ID=")) {
                    m.groupId = t.substr(10, t.length()-11);
                } else if (startsWith(t, "URI=")) {
                    m.uri = t.substr(5, t.length()-6);
                }
            }

            if (addThis) {
                if (!startsWith(m.uri, "http://") && !startsWith(m.uri, "https://")) {
                    // this is a relative URL -> construct absolute URL
                    auto last = useUri.find_last_of('/');
                    m.uri = useUri.substr(0, last+1).append(m.uri);
                }

                result.audio.push_back(m);
            }
        } else if (startsWith(line, starterIgnore)) {
            // this is already a usable m3u8. Don't process anymore but return a useful result
            result.width = 1920;
            result.height = 1080;
            result.url = useUri;
            return result;
        }
    }

    // remove all unwanted audio streams with wrong group-id
    if (!audioGroup.empty() && !result.audio.empty()) {
        auto it = result.audio.begin();
        while(it != result.audio.end()) {
            if(it->groupId != audioGroup) {
                it = result.audio.erase(it);
            } else {
                ++it;
            }
        }
    }

    result.width = maxW;
    result.height = maxH;
    result.url = m3uMax;

    result.channelName = "";
    result.vpid = -1;
    result.spid = -1;
    result.tpid = -1;
    result.apids.clear();

    return result;
}

void M3u8Handler::printStream(m3u_stream stream) {
    debug2("Url: %s\n", stream.url.c_str());
    debug2("   Width: %d, Height: %d\n", stream.width, stream.height);
    if (stream.audio.size() > 0) {
        debug2("   Audio:\n");
        for (auto s: stream.audio) {
            debug2("      Type: %s\n", s.type.c_str());
            debug2("      Lang: %s\n", s.language.c_str());
            debug2("      Name: %s\n", s.name.c_str());
            debug2("      Uri: %s\n", s.uri.c_str());
        }
    }

    debug2("   Channel information:\n");
    debug2("     Name: %s\n", stream.channelName.c_str());
    debug2("     VPid: %d\n", stream.vpid);
    debug2("     SPid: %d\n", stream.spid);
    debug2("     TPid: %d\n", stream.tpid);
    for (auto a : stream.apids) {
        debug2("     APid: %d\n", a);
    }
}