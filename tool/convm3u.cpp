#include <regex>
#include <sstream>
#include <iostream>
#include <set>
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

    FILE* output = fopen(outputFileName.c_str(), "wb");
    if (output == nullptr) {
        printf("Unable to open output file %s\n", outputFileName.c_str());
        exit(1);
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

    fclose(output);
}

void ConvM3U::createCompleteChannel(const std::vector<m3uEntry>& m3u, std::string transponderId, FILE *cfgOut, FILE *channelOut, std::string cfg, std::string radio) {
    std::string result_channel;
    std::string result_cfg;

    int cfgIdx = 1;
    int chanIdx = 10;
    int pidIdx = 200;
    for (auto m : m3u) {
        result_cfg.append(std::to_string(cfgIdx)).append(":").append(m.m3uUrl).append("\n");

        result_channel.append(m.tvgName).append(":").append(std::to_string(chanIdx)).append(":");
        result_channel.append("S=1|P=0|F=M3U|U=").append(cfg).append("|");
        result_channel.append("A=").append(std::to_string(cfgIdx)).append(":I:0:");
        result_channel.append(std::to_string(pidIdx)).append(":");
        for (int i = 1; i <= 5; ++i) {
            result_channel.append(std::to_string(pidIdx+i));
            if (i != 5) {
                result_channel.append(",");
            }
        }
        result_channel.append(":0:0:").append(std::to_string(cfgIdx)).append(":1:").append(transponderId).append(":");

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
    fwrite(result_cfg.c_str(), result_cfg.length(), 1, cfgOut);
}
