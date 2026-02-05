/*

    tcpClient.hpp

    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


    February 6, 2026, Bojan Jurca

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


#pragma once
#ifndef __TCP_CLIENT__
  #define __TCP_CLIENT__


  #include <WiFi.h>
  #include "tcpConnection.h"


  // TUNING PARAMETERS

  #ifndef SOCKET_TIMEOUT
    #define SOCKET_TIMEOUT (1)
  #endif

  #ifndef CONNECT_TIMEOUT
    #define CONNECT_TIMEOUT (10)
  #endif


  // missing function in LwIP
  inline const char *gai_strerror (int err);


  class tcpClient_t : public tcpConnection_t {

    private:
      const char *__errText__;

  public:
      tcpClient_t (const char *serverName, int serverPort);

      // error reporting
      inline operator bool () __attribute__((always_inline)) { return __errText__ != NULL; }
      inline const char *errText () __attribute__((always_inline)) { return __errText__; }
  };

#endif
