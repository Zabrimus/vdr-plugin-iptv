#include <regex>
#include <sstream>
#include "m3upipe.h"
#include "convm3u.h"

// all desired values
std::regex regs[] = {
        std::regex(".*(tvg-name)=\"(.*?)\".*"),
        std::regex(".*(tvg-id)=\"(.*?)\".*"),
        std::regex(".*(tvg-logo)=\"(.*?)\".*"),
        std::regex(".*(group-title)=\"(.*?)\".*"),
        std::regex(".*(m3u-title)=\"(.*?)\".*"),
        std::regex(".*(m3u-url)=\"(.*?)\".*")
};

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

ConvM3U::ConvM3U(std::string inputFileName, std::string outputFileName, std::string channelOutput) : outputFileName(outputFileName), inputFileName(inputFileName), channelOutput(channelOutput) {
}

ConvM3U::~ConvM3U() {
}

void ConvM3U::convert(std::string transponderId, std::string radio) {
    FILE* input = fopen(inputFileName.c_str(), "rb");
    if (input == nullptr) {
        printf("Unable to open input file %s\n", inputFileName.c_str());
        exit(1);
    }

    FILE *output = nullptr;
    if (radio == "0") {
        output = fopen(outputFileName.c_str(), "wb");
        if (output == nullptr) {
            printf("Unable to open output file %s\n", outputFileName.c_str());
            exit(1);
        }
    }

    FILE* outputChannel = fopen(channelOutput.c_str(), "wb");
    if (outputChannel == nullptr) {
        printf("Unable to open output file %s\n", channelOutput.c_str());
        exit(1);
    }

    printf("Process m3u file...\n");

    m3uContent = processM3U(input);
    fclose(input);

    printf("Extract data...\n");

    std::vector<m3uEntry> entries;

    std::istringstream inputStream{m3uContent};
    std::string line;
    while(std::getline(inputStream, line)) {
        m3uEntry entry;

        // extract interesting values per line
        for (const auto &r: regs) {
            std::smatch match;
            if (std::regex_search(line, match, r)) {
                std::string key = match[1].str();
                std::string value = match[2].str();

                if (key == "tvg-name") {
                    entry.tvgName = value;
                } else if (key == "tvg-id") {
                    entry.tvgId = value;
                } else if (key == "tvg-logo") {
                    entry.tvgLogo = value;
                } else if (key == "group-title") {
                    entry.groupTitle = value;
                } else if (key == "m3u-title") {
                    entry.m3uTitle = value;
                } else if (key == "m3u-url") {
                    entry.m3uUrl = value;
                }
            }
        }

        if (entry.groupTitle.empty()) {
            entry.groupTitle = "unknown";
        }

        if ((entry.m3uTitle.empty() && entry.tvgName.empty()) || strncmp(entry.m3uUrl.c_str(), "http", 4) != 0) {
            // ignore this entry
            continue;
        }

        entries.push_back(entry);
    }

    createCompleteChannel(entries, transponderId, output, outputChannel, outputFileName, radio);

    if (output != nullptr) {
        fclose(output);
    }

    if (outputChannel != nullptr) {
        fclose(outputChannel);
    }
}

void ConvM3U::createCompleteChannel(const std::vector<m3uEntry>& m3u, std::string transponderId, FILE *cfgOut, FILE *channelOut, std::string cfg, std::string radio) {
    std::string result_channel;
    std::string result_cfg;

    int cfgIdx = 1;
    int chanIdx = 10;
    int tid = atoi(transponderId.c_str());
    int pidIdx = 256;

    for (auto m : m3u) {
        result_cfg.append(std::to_string(cfgIdx)).append(":").append(m.m3uUrl).append("\n");

        result_channel.append(m.tvgName).append(":").append(std::to_string(chanIdx)).append(":");

        if (radio == "0") {
            result_channel.append("S=1|P=1|F=M3U|U=").append(cfg).append("|");
        } else {
            std::string url = m.m3uUrl;
            url = ReplaceAll(url, ":", "%3A");
            url = ReplaceAll(url, "|", "%7C");

            result_channel.append("S=1|P=0|F=RADIO|U=").append(url).append("|");
        }

        result_channel.append("A=").append(std::to_string(cfgIdx));

        if (!m.tvgId.empty()) {
            result_channel.append("|X=").append(m.tvgId);
        }

        result_channel.append(":I:0:");

        if (radio == "1") {
            result_channel.append("0:");
        } else {
            result_channel.append(std::to_string(256)).append(":");
        }

        // int maxApid = (radio == "1") ? 1 : 5;
        int maxApid = 1;

        for (int i = 1; i <= maxApid; ++i) {
            result_channel.append(std::to_string(257));
            if (i != maxApid) {
                result_channel.append(",");
            }
        }
        result_channel.append(":0:0:").append(std::to_string(cfgIdx)).append(":1:").append(std::to_string(tid)).append(":");

        tid++;

        if (radio == "0") {
            result_channel.append("0\n");
        } else {
            result_channel.append(std::to_string(cfgIdx)).append("\n");
        }

        cfgIdx++;
        chanIdx += 10;
        pidIdx += 10;
    }

    fwrite(result_channel.c_str(), result_channel.length(), 1, channelOut);

    if (radio == "0") {
        fwrite(result_cfg.c_str(), result_cfg.length(), 1, cfgOut);
    }
}
