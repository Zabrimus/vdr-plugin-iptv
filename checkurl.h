#pragma once

#include <thread>
#include <map>

class CheckURL {
private:
    std::thread checkThread;
    bool isRunning;
    std::map<std::string, std::string> channels;
    std::set<std::string> brokenChannels;

private:
    void executeChecks();

public:
    CheckURL();
    ~CheckURL();

    void start();
    void abort();
};

