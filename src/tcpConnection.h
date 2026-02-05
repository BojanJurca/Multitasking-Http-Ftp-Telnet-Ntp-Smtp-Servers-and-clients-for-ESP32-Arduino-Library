  /*

    tcpConnection.hpp

    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library

  
    February 6, 2026, Bojan Jurca


    Classes implemented/used in this module:

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
#ifndef __TCP_CONNECTION__
    #define __TCP_CONNECTION__


    #include <WiFi.h>
    #include <lwip/netdb.h>
    #include <LwIpMutex.h>


    // singelton network traffic declaration
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

        public:
            tcpConnection_t ();
            tcpConnection_t (int connectionSocket, char *clientIP, char *serverIP);
            ~tcpConnection_t ();

            // bool() operator to test if tcpConnection is ready
            inline operator bool () __attribute__((always_inline)) { return __connectionSocket__ != -1; }

            int recv (void *buf, size_t len);
            int recvBlock (void *buf, size_t len);
            int recvString (char *buf, size_t len, const char *endingString);
            int peek (void *buf, size_t len);
            int sendBlock (void *buf, size_t len);
            int sendString (const char *buf);

            void close ();

            inline int  getSocket () __attribute__((always_inline)) { return __connectionSocket__; }
            inline char *getClientIP () __attribute__((always_inline)) { return __clientIP__; }
            inline char *getServerIP () __attribute__((always_inline)) { return __serverIP__; }

            inline time_t getIdleTimeout () __attribute__((always_inline)) { return __idleTimeout__; }
            inline void setIdleTimeout (time_t seconds) __attribute__((always_inline)) { __idleTimeout__ = seconds; }
            inline void stillActive () __attribute__((always_inline)) { __lastActive__ = millis (); }
            inline bool idleTimeout () __attribute__((always_inline)) { return __idleTimeout__ == 0 ? 0 : millis () - __lastActive__ > __idleTimeout__ * 1000; }


        protected:
            int __connectionSocket__ = -1;
            time_t __idleTimeout__ = 0;
            unsigned long __lastActive__ = 0;

            char __clientIP__ [INET6_ADDRSTRLEN] = {};
            char __serverIP__ [INET6_ADDRSTRLEN] = {};
    };

#endif