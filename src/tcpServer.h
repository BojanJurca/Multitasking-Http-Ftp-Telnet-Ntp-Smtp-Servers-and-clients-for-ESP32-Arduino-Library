/*

    tcpServer.cpp

    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library

  
    February 6, 2026, Bojan Jurca


    Classes implemented/used in this module:

        tcpServer_t

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
#ifndef __TCP_SERVER__
  #define __TCP_SERVER__


  #include <WiFi.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <fcntl.h>
  #include "LwIpMutex.h"
  #include "tcpConnection.h"


  #define EAGAIN 11
  #define ENAVAIL 119


  // TUNING PARAMETERS

  #ifndef TCP_LISTENER_STACK_SIZE
    #define TCP_LISTENER_STACK_SIZE (2 * 1024 + 512)
  #endif

  #ifndef SOCKET_TIMEOUT
    #define SOCKET_TIMEOUT (1)
  #endif


  extern int __runningTcpConnections__;

  class tcpServer_t {

    public:

        tcpServer_t (int serverPort,
                     bool (*firewallCallback) (char *clientIP, char *serverIP),
                     bool runListenerInItsOwnTask = true);

        virtual ~tcpServer_t ();

        // bool() operator to test if tcpServer started successfully
        inline operator bool () __attribute__((always_inline)) { return __state__ == RUNNING; }

        // accepts incoming connection
        virtual tcpConnection_t *accept ();

    private:

        int __serverPort__;

        bool (*__firewallCallback__) (char *clientIP, char *serverIP);

        enum STATE_TYPE { STARTING = 0, NOT_RUNNING = 1, RUNNING = 2 } __state__ = STARTING;

        int __listeningSocket__ = -1;

        bool __runListenerInItsOwnTask__;

        virtual tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP);

  };

#endif
