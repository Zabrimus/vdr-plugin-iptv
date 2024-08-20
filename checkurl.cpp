#include <vdr/channels.h>
#include <curl/curl.h>
#include "checkurl.h"
#include "source.h"
#include "protocolm3u.h"

#define READLIMIT_BYTES  (1024 * 50)
#define READ_TIMEOUT_SEC 10L

struct memory {
    char *response;
    size_t size;
};

CheckURL::CheckURL() {
    isRunning = false;
}

CheckURL::~CheckURL() {
    abort();
}

void CheckURL::start() {
    if (isRunning) {
        return;
    }

    isRunning = true;
    checkThread = std::thread(&CheckURL::executeChecks, this);
}

void CheckURL::abort() {
    isRunning = false;
    checkThread.join();
}


static size_t check_header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    // ignore the header;
    return nitems * size;
}

size_t fullsizeRead = 0;
static size_t check_body_callback(char *data, size_t size, size_t nmemb, void *clientp)
{
    // ignore body
    size_t realsize = size * nmemb;
    fullsizeRead += realsize;

    if (fullsizeRead > READLIMIT_BYTES) {
        return -1;
    } else {
        return realsize;
    }
}

void CheckURL::executeChecks() {
    channels.clear();

    // collect list of all channels and URLs
    {
        LOCK_CHANNELS_READ;
        for (const cChannel *Channel = Channels->First(); Channel; Channel = Channels->Next(Channel)) {
            if (Channel->IsSourceType('I')) {
                std::string u;
                cIptvTransponderParameters itp(Channel->Parameters());

                switch (itp.Protocol()) {
                case cIptvTransponderParameters::eProtocolM3U:
                    u = cIptvProtocolM3U::findUrl(itp.Parameter(), itp.Address());
                    break;

                case cIptvTransponderParameters::eProtocolRadio:
                case cIptvTransponderParameters::eProtocolStream:
                    u = itp.Address();
                    break;

                default:
                    // currently not supported
                    break;
                }

                if (!u.empty()) {
                    u = ReplaceAll(u, "%3A", ":");
                    u = ReplaceAll(u, "%7C", "|");

                }
                channels[*Channel->GetChannelID().ToString()] = u;
            }
        }
    }

    // iterate over all channels and checks the URL
    CURLcode ret;
    CURL *hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_HEADER, 1);
    // curl_easy_setopt(hnd, CURLOPT_NOBODY, 1);
    curl_easy_setopt(hnd, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(hnd, CURLOPT_HEADERFUNCTION, check_header_callback);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, check_body_callback);
    curl_easy_setopt(hnd, CURLOPT_TIMEOUT, READ_TIMEOUT_SEC);

    struct memory chunk = {nullptr};
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void *)&chunk);

    for (auto a : channels) {
        if (!isRunning) {
            break;
        }
        fullsizeRead = 0;
        curl_easy_setopt(hnd, CURLOPT_URL, a.second.c_str());

        ret = curl_easy_perform(hnd);
        free(chunk.response);
        chunk.response = nullptr;
        chunk.size = 0;

        long response_code;
        curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &response_code);

        if (!((ret == CURLE_OK || fullsizeRead >= READLIMIT_BYTES) && (response_code < 400))) {
            printf("Error: %ld: %s -> %s\n", response_code, a.first.c_str(), a.second.c_str());
            brokenChannels.emplace(a.first.c_str());

            /*
            {
                LOCK_CHANNELS_READ;
                tChannelID testChannelID = tChannelID::FromString(a.first.c_str());
                const cChannel *testChannel = Channels->GetByChannelID(testChannelID);
                cDevice::PrimaryDevice()->SwitchChannel(testChannel, true);
            }
            */
        }
    }

    curl_easy_cleanup(hnd);

    isRunning = false;
}
