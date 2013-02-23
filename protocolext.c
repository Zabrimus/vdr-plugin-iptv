/*
 * protocolext.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>
#include <vdr/plugin.h>

#include "common.h"
#include "config.h"
#include "protocolext.h"

#ifndef EXTSHELL
#define EXTSHELL "/bin/bash"
#endif

cIptvProtocolExt::cIptvProtocolExt()
: pidM(-1),
  scriptFileM(""),
  scriptParameterM(0),
  streamPortM(0)
{
  debug("cIptvProtocolExt::cIptvProtocolExt()\n");
}

cIptvProtocolExt::~cIptvProtocolExt()
{
  debug("cIptvProtocolExt::~cIptvProtocolExt()\n");
  // Drop the socket connection
  cIptvProtocolExt::Close();
}

void cIptvProtocolExt::ExecuteScript(void)
{
  debug("cIptvProtocolExt::ExecuteScript()\n");
  // Check if already executing
  if (isActive || isempty(scriptFileM))
     return;
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
     for (int i = STDERR_FILENO + 1; i < MaxPossibleFileDescriptors; i++)
         close(i);
     // Execute the external script
     cString cmd = cString::sprintf("%s %d %d", *scriptFileM, scriptParameterM, streamPortM);
     debug("cIptvProtocolExt::ExecuteScript(child): %s\n", *cmd);
     // Create a new session for a process group
     ERROR_IF_RET(setsid() == -1, "setsid()", _exit(-1));
     if (execl(EXTSHELL, "sh", "-c", *cmd, (char *)NULL) == -1) {
        error("Script execution failed: %s", *cmd);
        _exit(-1);
        }
     _exit(0);
     }
  else {
     debug("cIptvProtocolExt::ExecuteScript(): pid=%d\n", pidM);
     }
}

void cIptvProtocolExt::TerminateScript(void)
{
  debug("cIptvProtocolExt::TerminateScript(): pid=%d\n", pidM);
  if (!isActive || isempty(scriptFileM))
     return;
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
       if ((waitms % 2000) == 0) {
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
       memset(&waitStatus, '\0', sizeof(waitStatus));
       retval = waitid(P_PID, pidM, &waitStatus, (WNOHANG | WEXITED));
#endif // __FreeBSD__
       ERROR_IF_RET(retval < 0, "waitid()", waitOver = true);
       // These are the acceptable conditions under which child exit is
       // regarded as successful
#ifdef __FreeBSD__
       if (retval > 0 && (WIFEXITED(waitStatus) || WIFSIGNALED(waitStatus))) {
#else  // __FreeBSD__
       if (!retval && waitStatus.si_pid && (waitStatus.si_pid == pidM) &&
          ((waitStatus.si_code == CLD_EXITED) || (waitStatus.si_code == CLD_KILLED))) {
#endif // __FreeBSD__
          debug("Child (%d) exited as expected\n", pidM);
          waitOver = true;
          }
       // Unsuccessful wait, avoid busy looping
       if (!waitOver)
          cCondWait::SleepMs(timeoutms);
       }
     pidM = -1;
     }
}

bool cIptvProtocolExt::Open(void)
{
  debug("cIptvProtocolExt::Open()\n");
  // Reject empty script files
  if (!strlen(*scriptFileM))
     return false;
  // Create the listening socket
  OpenSocket(streamPortM);
  // Execute the external script
  ExecuteScript();
  isActive = true;
  return true;
}

bool cIptvProtocolExt::Close(void)
{
  debug("cIptvProtocolExt::Close()\n");
  // Terminate the external script
  TerminateScript();
  isActive = false;
  // Close the socket
  CloseSocket();
  return true;
}

int cIptvProtocolExt::Read(unsigned char* bufferAddrP, unsigned int bufferLenP)
{
  return cIptvUdpSocket::Read(bufferAddrP, bufferLenP);
}

bool cIptvProtocolExt::Set(const char* locationP, const int parameterP, const int indexP)
{
  debug("cIptvProtocolExt::Set('%s', %d, %d)\n", locationP, parameterP, indexP);
  if (!isempty(locationP)) {
     struct stat stbuf;
     // Update script file and parameter
     scriptFileM = cString::sprintf("%s/%s", IptvConfig.GetConfigDirectory(), locationP);
     if ((stat(*scriptFileM, &stbuf) != 0) || (strstr(*scriptFileM, "..") != 0)) {
        error("Non-existent or relative path script '%s'", *scriptFileM);
        return false;
        }
     scriptParameterM = parameterP;
     // Update listen port
     streamPortM = IptvConfig.GetExtProtocolBasePort() + indexP;
     }
  return true;
}

cString cIptvProtocolExt::GetInformation(void)
{
  //debug("cIptvProtocolExt::GetInformation()");
  return cString::sprintf("ext://%s:%d", *scriptFileM, scriptParameterM);
}
