/*
 * protocolext.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>
#include <thread>

#include "common.h"
#include "config.h"
#include "log.h"
#include "protocolextt.h"

#ifndef EXTSHELL
#define EXTSHELL "/bin/bash"
#endif

cIptvProtocolExtT::cIptvProtocolExtT()
    : pidM(-1),
      scriptFileM(""),
      scriptParameterM(0) {

    debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolExtT::~cIptvProtocolExtT() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Drop the socket connection
    cIptvProtocolExtT::Close();
}

void cIptvProtocolExtT::ExecuteScript() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Check if already executing
    if (isActiveM || isempty(scriptFileM)) {
        return;
    }

    if (pidM > 0) {
        error("Cannot execute script!");
        return;
    }

    // Let's fork
    ERROR_IF_RET((pidM = fork()) == -1, "fork()", return);
    // Check if child process
    if (pidM == 0) {
        // Close all dup'ed filedescriptors
        int MaxPossibleFileDescriptors = getdtablesize();

        for (int i = STDERR_FILENO + 1; i < MaxPossibleFileDescriptors; i++) {
            close(i);
        }

        // Execute the external script
        cString cmd = cString::sprintf("%s %d %d", *scriptFileM, scriptParameterM, streamPortM);

        debug1("%s Child %s", __PRETTY_FUNCTION__, *cmd);

        // Create a new session for a process group
        ERROR_IF_RET(setsid()==-1, "setsid()", _exit(-1));
        if (execl(EXTSHELL, "sh", "-c", *cmd, (char *) nullptr) == -1) {
            error("Script execution failed: %s", *cmd);
            _exit(-1);
        }

        _exit(0);
    } else {
        debug1("%s pid=%d", __PRETTY_FUNCTION__, pidM);
    }
}

void cIptvProtocolExtT::TerminateScript() {
    debug1("%s pid=%d", __PRETTY_FUNCTION__, pidM);

    if (!isActiveM || isempty(scriptFileM)) {
        return;
    }

    if (pidM > 0) {
        const unsigned int timeoutms = 100;
        unsigned int waitms = 0;
        bool waitOver = false;

        // Signal and wait for termination
        int retval = killpg(pidM, SIGINT);
        ERROR_IF_RET(retval < 0, "kill()", waitOver = true);

        while (!waitOver) {
            retval = 0;
            waitms += timeoutms;

            if ((waitms%2000)==0) {
                error("Script '%s' won't terminate - killing it!", *scriptFileM);
                killpg(pidM, SIGKILL);
            }

            // Clear wait status to make sure child exit status is accessible
            // and wait for child termination
#ifdef __FreeBSD__
            int waitStatus = 0;
            retval = waitpid(pidM, &waitStatus, WNOHANG);
#else  // __FreeBSD__
            siginfo_t waitStatus;
            memset(&waitStatus, 0, sizeof(waitStatus));
            retval = waitid(P_PID, pidM, &waitStatus, (WNOHANG | WEXITED));
#endif // __FreeBSD__
            ERROR_IF_RET(retval < 0, "waitid()", waitOver = true);
            // These are the acceptable conditions under which child exit is
            // regarded as successful
#ifdef __FreeBSD__
            if (retval > 0 && (WIFEXITED(waitStatus) || WIFSIGNALED(waitStatus))) {
#else  // __FreeBSD__
            if (!retval && waitStatus.si_pid && (waitStatus.si_pid==pidM) &&
                ((waitStatus.si_code==CLD_EXITED) || (waitStatus.si_code==CLD_KILLED))) {
#endif // __FreeBSD__
                debug1("%s Child (%d) exited as expected", __PRETTY_FUNCTION__, pidM);

                waitOver = true;
            }

            // Unsuccessful wait, avoid busy looping
            if (!waitOver)
                cCondWait::SleepMs(timeoutms);
        }
        pidM = -1;
    }
}

bool cIptvProtocolExtT::Open() {
    if (isActiveM) {
        // the script is still running
        return true;
    }

    debug1("%s", __PRETTY_FUNCTION__);

    // Reject empty script files
    if (!strlen(*scriptFileM)) {
        return false;
    }

    // Create the listening socket
    OpenSocket(streamPortM);

    // Execute the external script
    ExecuteScript();
    isActiveM = true;

    // Wait for client
    int retries = 0;
    int waitMs = 200;
    while (!Accept() && retries < 1000 / waitMs * 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
        retries++;
    }

    return ClientConnected();
}

bool cIptvProtocolExtT::Close() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Terminate the external script
    TerminateScript();
    isActiveM = false;

    // Close the socket
    CloseSocket();

    return true;
}

int cIptvProtocolExtT::Read(unsigned char *bufferAddrP, unsigned int bufferLenP) {
    return cIptvTcpServerSocket::Read(bufferAddrP, bufferLenP);
}

bool
cIptvProtocolExtT::SetSource(SourceParameter parameter) {
    debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, parameter.locationP, parameter.parameterP, parameter.indexP);

    if (!isempty(parameter.locationP)) {
        struct stat stbuf;
        // Update script file and parameter
        scriptFileM = cString::sprintf("%s/%s", IptvConfig.GetResourceDirectory(), parameter.locationP);

        if ((stat(*scriptFileM, &stbuf) != 0) || (strstr(*scriptFileM, "..") != nullptr)) {
            error("Non-existent or relative path script '%s'", *scriptFileM);

            return false;
        }

        scriptParameterM = parameter.parameterP;
        // Update listen port
        streamPortM = IptvConfig.GetProtocolBasePort() + parameter.indexP*2;
    }
    return true;
}

bool cIptvProtocolExtT::SetPid(int pidP, int typeP, bool onP) {
    debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);

    return true;
}

cString cIptvProtocolExtT::GetInformation() {
    debug16("%s", __PRETTY_FUNCTION__);

    return cString::sprintf("ext://%s:%d", *scriptFileM, scriptParameterM);
}
