/*

    tcpServer.h
  
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  

    May 22, 2026, Bojan Jurca


    Multitasking/thread-safe classes and functions: 

        inherits from:  ─────────▶                                                                                          ┌───────────────────┐
        uses:           •••••••••▶                                                                                           │ ntpClient_t       │
                                                                                                                             └───────────────────┘
                                                                                                                             ┌───────────────────┐
                                                                                        ••••••••••••••••••••••••••••••••••••▶│ httpsClient       │
                                                                                        •                                    └───────────────────┘
                                                                                        •                                    ┌───────────────────┐
                                                                                        •                           ••••••••▶│ httpClient        │
                                                                                        •                           •        └───────────────────┘
                                                                                        •                           •        ┌───────────────────┐
                                                                                        •                           ••••••••▶│ smtpClient        │
                                                                                        •                           •        └───────────────────┘
                                                                                        •                           •                                             
                                                                                        •                           •                             
┌──────────────────────┐                                                                •                  ┌────────•──────────┐                  
│ tcpServer_t          │••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••▶│ tcpConnection_t   │                  
└──────────────────────┘                                    •                           •               •  └───────────────────┘                  
           ▲                                                •                           •               •            ▲                            
           │                                                •                           •               •            │                            
           │  ┌─────────────────┐                           •                ┌──────────────────────┐   •            │                            
           │──│ httpServer_t    │ ••••••••••••••••••••••••••••••••••••••••••▶│ tlsConnection_t      │────────────────┘                            
           │  └─────────────────┘                           •                └──────────────────────┘   •            │                            
           │           ▲                                    •                           ▲               •            │                            
           │           │                                    •                           •               •            │                            
           │           │                                    •           ┌───────────────────────────┐   •            │                            
           │  ┌──────────────────┐                          •           │ httpServer_t::webSocket_t │••••            │                            
           │  │ httpsServer_t    │                          •           └───────────────────────────┘                │                            
           │  └──────────────────┘                          •                           ▲                            │                            
           │                                                •                           │                            │                            
           │                                                •           ┌────────────────────────────────┐           │                            
           │                                                •           │ httpServer_t::httpConnection_t │           │                            
           │                                                •           └────────────────────────────────┘           │                            
           │                                                •                                                        │                            
           │  ┌──────────────────┐                          •           ┌────────────────────────────────────┐       │                            
           │──│ ftpServer_t      │•••••••••••••••••••••••••••••••••••••▶│ftpServer_t::ftpControlConnection_t │──────│                            
           │  └──────────────────┘                                      └────────────────────────────────────┘       │                            
           │                                                                                                         │                                                   
           │  ┌──────────────────┐                                      ┌───────────────────────────────────┐        │                            
           └──│ telnetServer_t   │•••••••••••••••••••••••••••••••••••••▶│telnetServer_t::telnetConnection_t │───────┘
              └──────────────────┘                                      └───────────────────────────────────┘                                    


Edit/view: https://cascii.app/e83d5                           

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
    #define TCP_LISTENER_STACK_SIZE (3 * 1024)
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
