/*
 * socket.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <arpa/inet.h>
#ifdef __FreeBSD__
#include <netinet/in.h>
#endif // __FreeBSD__

class cIptvSocket {
private:
    int socketPortM;

protected:
    enum {
        eReportIntervalS = 300 // in seconds
    };

    int socketDescM;
    struct sockaddr_in sockAddrM;
    time_t lastErrorReportM;
    int packetErrorsM;
    int sequenceNumberM;
    bool isActiveM;

protected:
    bool OpenSocket(int portP, bool isUdpP);
    void CloseSocket();
    bool CheckAddress(const char *addrP, in_addr_t *inAddrP);

public:
    cIptvSocket();
    virtual ~cIptvSocket();
};

class cIptvUdpSocket : public cIptvSocket {
private:
    in_addr_t streamAddrM;
    in_addr_t sourceAddrM;
    bool useIGMPv3M;

public:
    cIptvUdpSocket();
    ~cIptvUdpSocket() override;
    virtual int Read(unsigned char *bufferAddrP, unsigned int bufferLenP);
    bool OpenSocket(int Port);
    bool OpenSocket(int Port, const char *streamAddrP, const char *sourceAddrP, bool useIGMPv3P);
    void CloseSocket();
    bool JoinMulticast();
    bool DropMulticast();
};

class cIptvTcpSocket : public cIptvSocket {
public:
    cIptvTcpSocket();
    ~cIptvTcpSocket() override;
    virtual int Read(unsigned char *bufferAddrP, unsigned int bufferLenP);
    bool OpenSocket(int portP, const char *streamAddrP);
    void CloseSocket();
    bool ConnectSocket();
    bool ReadChar(char *bufferAddrP, unsigned int timeoutMsP);
    bool Write(const char *bufferAddrP, unsigned int bufferLenP);
};

