#pragma once

#include <thread>
#include <map>
#include <set>

class CheckURL {
private:
    std::thread checkThread;
    bool isRunning;
    std::map<std::string, std::string> channels;
    std::set<std::string> brokenChannels;

private:
    void executeChecks();

    int countChannels;
    int checkedChannels;
    cString currentChannelId;
    cString currentChannelUrl;

public:
    CheckURL();
    ~CheckURL();

    void start();
    cString status();
    void stop();

    int getCountChannels() const { return countChannels; };
    int getCheckedChannels() const { return checkedChannels; };
    cString getCurrentChannelId() { return currentChannelId; };
    cString getCurrentChannelUrl() { return currentChannelUrl; };
};

