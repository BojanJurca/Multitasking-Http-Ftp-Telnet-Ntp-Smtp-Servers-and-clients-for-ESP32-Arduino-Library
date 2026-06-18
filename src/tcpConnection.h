/*

    tcpConnection.h 
  
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
#ifndef __TCP_CONNECTION__
    #define __TCP_CONNECTION__


    #include <WiFi.h>
    #include <lwip/netdb.h>
    #include <LwIpMutex.h>
    #include "gai_strerror.h"


    // TUNING PARAMETERS

    #ifndef SOCKET_TIMEOUT
        #define SOCKET_TIMEOUT (1)
    #endif

    #ifndef CONNECT_TIMEOUT
        #define CONNECT_TIMEOUT (10)
    #endif


    // singleton network traffic declaration
    struct networkTrafficData_t {
        unsigned long bytesReceived;
        unsigned long bytesSent;
    };    
    struct networkTraffic_t {
        unsigned long bytesReceived;
        unsigned long bytesSent;
        networkTrafficData_t perSocket [MEMP_NUM_NETCONN];
        networkTrafficData_t& operator [] (int sockfd);
    };
    inline networkTraffic_t& networkTraffic () {
        static networkTraffic_t instance {};
        return instance;
    }
    inline networkTrafficData_t& networkTraffic_t::operator [] (int sockfd) {
        return perSocket [sockfd - LWIP_SOCKET_OFFSET];
    }


    class tcpConnection_t {
        friend class tlsConnection_t;

        private:
            const char *__errText__ = "";

        public:
            tcpConnection_t ();
            // server connection constructor
            tcpConnection_t (int connectionSocket, const char *clientIP, const char *serverIP);
            // client connection constructor
            tcpConnection_t (const char *serverName, int serverPort);
            virtual ~tcpConnection_t ();

            // bool() operator to test if tcpConnection is ready
            operator bool () const { return __connectionSocket__ != -1; }
            inline const char *errText () __attribute__((always_inline)) { return __errText__; }

            int recv (void *buf, size_t len);
            int send (void *buf, size_t len);
            virtual int recvBlock (void *buf, size_t len);
            virtual int recvString (char *buf, size_t len, const char *endingString);
            virtual int peek (void *buf, size_t len);
            virtual int sendBlock (void *buf, size_t len);
            virtual int sendString (const char *buf);

            void close ();

            inline int  getSocket () __attribute__((always_inline)) { return __connectionSocket__; }
            inline char *getClientIP () __attribute__((always_inline)) { return __clientIP__; }
            inline char *getServerIP () __attribute__((always_inline)) { return __serverIP__; }

            inline time_t getIdleTimeout () __attribute__((always_inline)) { return __idleTimeout__; }
            inline void setIdleTimeout (time_t seconds) __attribute__((always_inline)) { __idleTimeout__ = seconds; }
            inline void stillActive () __attribute__((always_inline)) { __lastActive__ = millis (); }
            inline bool idleTimeout () __attribute__((always_inline)) { return __idleTimeout__ == 0 ? 0 : (millis () - __lastActive__ > __idleTimeout__ * 1000); }

        protected:
            int __connectionSocket__ = -1;
            time_t __idleTimeout__ = 0;
            unsigned long __lastActive__ = 0;

            char __clientIP__ [INET6_ADDRSTRLEN] = {};
            char __serverIP__ [INET6_ADDRSTRLEN] = {};
    };

#endif