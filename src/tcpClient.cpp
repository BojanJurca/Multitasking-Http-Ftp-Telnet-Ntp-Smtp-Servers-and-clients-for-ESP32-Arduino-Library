/*

    tcpClient.hpp

    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


    January 1, 2026, Bojan Jurca

    Classes implemented/used in this module:

        tcpServer_t
        tcpConnection_t

    Inheritance diagram:    

             ┌─────────────┐
             │ tcpServer_t ┼┐
             └─────────────┘│
                            │
      ┌─────────────────┐   │
      │ tcpConnection_t ┼┐  │
      └─────────────────┘│  │
                         │  │
                         │  │  ┌─────────────────────────┐
                         │  ├──┼─ httpServer_t           │
                         ├─────┼──── webSocket_t         │
                         │  │  │     └── httpConection_t │
                         │  │  └─────────────────────────┘
                         │  │  logicly webSocket_t would inherit from httpConnection_t but it is easier to implement it the other way around
                         │  │
                         │  │
                         │  │  ┌────────────────────────┐
                         │  ├──┼─ telnetServer_t        │
                         ├─────┼──── telnetConnection_t │
                         │  │  └────────────────────────┘
                         │  │
                         │  │
                         │  │  ┌────────────────────────────┐
                         │  └──┼─ ftpServer_t               │
                         ├─────┼──── ftpControlConnection_t │
                         │     └────────────────────────────┘
                         │  
                         │  
                         │     ┌──────────────┐
                         └─────┼─ tcpClient_t │
                               └──────────────┘

*/


#include <WiFi.h>
#include "tcpClient.h"
#include <WiFi.h>
#include <errno.h>
#include <string.h>


// missing function in LwIP
static const char *gai_strerror (int err) {
    switch (err) {
        case EAI_AGAIN:     return "temporary failure in name resolution";
        case EAI_BADFLAGS:  return "invalid value for ai_flags field";
        case EAI_FAIL:      return "non-recoverable failure in name resolution";
        case EAI_FAMILY:    return "ai_family not supported";
        case EAI_MEMORY:    return "memory allocation failure";
        case EAI_NONAME:    return "name or service not known";
        case EAI_SERVICE:   return "service not supported for ai_socktype";
        case EAI_SOCKTYPE:  return "ai_socktype not supported";
        default:            return "invalid gai_errno code";
    }
}


tcpClient_t::tcpClient_t (const char *serverName, int serverPort) : tcpConnection_t () {
    __errText__ = NULL;

    if (!WiFi.isConnected () || WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // esp32 can crash without this check
      __errText__ = "not connected";
     	getLogQueue () << "[tcpClient] " << "not connected" << endl;
      return;
    }

    // resolve server name
    struct addrinfo hints, *res, *p;
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
      int status = getaddrinfo (serverName, NULL, &hints, &res);
    xSemaphoreGive (getLwIpMutex ());
    if (status != 0) {
      __errText__ = gai_strerror (status);
      getLogQueue () << "[tcpClient] " << __errText__ << endl;
      return;
    }

    // get IP addresses for serverName
    bool isIPv6 = false;
    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        if (p->ai_family == AF_INET) {
            isIPv6 = false;
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else {
            isIPv6 = true;
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        inet_ntop (p->ai_family, addr, __serverIP__, sizeof (__serverIP__));
        break;
    }
    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
      freeaddrinfo (res);
    // xSemaphoreGive (getLwIpMutex ());

    // create socket
    // xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
      __connectionSocket__ = socket (isIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
      if (__connectionSocket__ < 0) {
        __errText__ = strerror (errno);
        getLogQueue () << "[tcpClient] " << __errText__ << endl;
        xSemaphoreGive (getLwIpMutex ());
        return;
      }

      // get client's IP address
      struct sockaddr_storage thisAddress = {};
      socklen_t len = sizeof (thisAddress);
      if (getsockname (__connectionSocket__, (struct sockaddr *) &thisAddress, &len) != -1) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *) &thisAddress;
        inet_ntop (AF_INET6, &s->sin6_addr, __clientIP__, sizeof (__clientIP__));
        if (!isIPv6) {
          char *p = __clientIP__;
          for (char *q = p; *q; q++)
            if (*q == ':')
              p = q;
          if (p != __clientIP__)
            strcpy (__clientIP__, p + 1);
        }
      }

      // make connection socket non-blocking
      if (fcntl (__connectionSocket__, F_SETFL, O_NONBLOCK) < 0) {
        __errText__ = strerror (errno);
        getLogQueue () << "[tcpClient] " << __errText__ << endl;
        xSemaphoreGive (getLwIpMutex ());
        return;
      }

      // connect to the server
      if (isIPv6) {

        struct sockaddr_in6 server_addr = {};
        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_len = sizeof(server_addr);

        if (inet_pton (AF_INET6, __serverIP__, &server_addr.sin6_addr) <= 0) {
          __errText__ = "invalid network address";
          getLogQueue () << "[tcpClient] " << __errText__ << " " << __serverIP__ << endl;
        } else {
          server_addr.sin6_port = htons (serverPort);
          if (connect (__connectionSocket__, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
            if (errno != EINPROGRESS) {
              __errText__ = strerror (errno);
              getLogQueue () << "[tcpClient] " << __errText__ << endl;
              ::close (__connectionSocket__);
              __connectionSocket__ = -1;
            }
          } // if connect == 0 or errno == EINPROGRESS everithing is fine so far
        } 

      } else {

        struct sockaddr_in server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_len = sizeof (server_addr);

        if (inet_pton (AF_INET, __serverIP__, &server_addr.sin_addr) <= 0) {
          __errText__ = "invalid network address";
          getLogQueue () << "[tcpClient] " << __errText__ << " " << __serverIP__ << endl;
        } else {
          server_addr.sin_port = htons (serverPort);
          if (connect (__connectionSocket__, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
            if (errno != EINPROGRESS) {
              __errText__ = strerror (errno);
              getLogQueue () << "[tcpClient] " << __errText__ << endl;
              ::close (__connectionSocket__);
              __connectionSocket__ = -1;
            }
          } // if connect == 0 or errno == EINPROGRESS everithing is fine so far
        } 
      
      }

    xSemaphoreGive (getLwIpMutex ());

    if (__connectionSocket__ == -1)
      return;

    // wait until connected or time-out
    unsigned long startMillis = millis ();
    bool connected = false;
    while (!connected) {
      delay (25);
      if (millis () - startMillis > CONNECT_TIMEOUT * 1000) {
        __errText__ = "connect time-out";
        getLogQueue () << "[tcpClient] " << __errText__ << endl;
        xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);          
          ::close (__connectionSocket__);
          __connectionSocket__ = -1;
        xSemaphoreGive (getLwIpMutex ());
        return;
      }

      // wait for socket
      fd_set wfds;
      FD_ZERO (&wfds);
      FD_SET (__connectionSocket__, &wfds);
      struct timeval tv = { 0, 200000 };
      xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
        switch (select (__connectionSocket__ + 1, NULL, &wfds, NULL, &tv)) {
          case -1:  // break;
                    __errText__ = strerror (errno);
                    getLogQueue () << "[tcpClient] " << __errText__ << endl;
                    ::close (__connectionSocket__);
                    __connectionSocket__ = -1;
                    xSemaphoreGive (getLwIpMutex ());
                    return;          
          case 0:   // socket time-out (which is not idle time-out)
                    break;
          default:
                    // check if socket is writable
                    int err;
                    socklen_t len = sizeof (err);
                    getsockopt (__connectionSocket__, SOL_SOCKET, SO_ERROR, &err, &len);
                    connected = (err == 0);
                    /*
                    if (FD_ISSET (__connectionSocket__, &wfds)) {
                        int err = 0;
                        socklen_t len = sizeof (err);
                        getsockopt (__connectionSocket__, SOL_SOCKET, SO_ERROR, &err, &len);
                        if (err == 0) {
                          connected = true;
                        } else {
                          __errText__ = strerror (err);
                          ::close (__connectionSocket__);
                          __connectionSocket__ = -1;
                          xSemaphoreGive (getLwIpMutex());
                          return;
                        }
                      }
                      */
                      break;
        }
      xSemaphoreGive (getLwIpMutex ());
    } // while
    
  // set socket time-out (without error checking, this is just a back-up option)
  xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
    // set socket time-out (without error checking, this is just a back-up option)
    struct timeval tv = { SOCKET_TIMEOUT, 0 };
    setsockopt (__connectionSocket__, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof (tv));
    setsockopt (__connectionSocket__, SOL_SOCKET, SO_SNDTIMEO, (const char *) &tv, sizeof (tv));
  xSemaphoreGive (getLwIpMutex ());

  networkTraffic () [__connectionSocket__] = {0, 0};
}
