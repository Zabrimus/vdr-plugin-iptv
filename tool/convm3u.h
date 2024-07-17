#pragma once

#include <string>
#include <vector>
#include <set>

struct m3uEntry {
    std::string tvgName;
    std::string tvgId;
    std::string tvgLogo;
    std::string groupTitle;
    std::string m3uTitle;
    std::string m3uUrl;
};

class ConvM3U {
public:
    ConvM3U(std::string inputFileName, std::string outputFileName, std::string channelOutput);
    ~ConvM3U();

    void convert(std::string transponderId, std::string radio);

private:
    void createCompleteChannel(const std::vector<m3uEntry>& m3u, std::string transponderId, FILE *cfgOut, FILE *channelOut, std::string cfg, std::string radio);

private:
    std::string m3uContent;
    std::string outputFileName;
    std::string channelOutput;
    std::string inputFileName;
    std::string radio;
};