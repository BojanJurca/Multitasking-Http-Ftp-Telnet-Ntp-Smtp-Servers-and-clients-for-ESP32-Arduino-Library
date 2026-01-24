  /*

    tcpConnection.hpp

    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


    Dec 25, 2025, Bojan Jurca


    Classes implemented/used in this module

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
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "tcpConnection.h"
#include <dmesg.hpp>
#include <ostream.hpp>


tcpConnection_t::tcpConnection_t () {}
tcpConnection_t::tcpConnection_t (int connectionSocket, char *clientIP, char *serverIP) {
    __connectionSocket__ = connectionSocket;
    strncpy (__clientIP__, clientIP, sizeof (__clientIP__) - 1);
    strncpy (__serverIP__, serverIP, sizeof (__serverIP__) - 1);
    networkTraffic () [__connectionSocket__] = {0, 0};

    // make connection socket non-blocking
    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);    
        if (fcntl (__connectionSocket__, F_SETFL, O_NONBLOCK) < 0) {
            cout << ( dmesgQueue << "[tcpConn] " << "error: " << errno << " " << strerror (errno) ) << endl;
            xSemaphoreGive (getLwIpMutex ());
            close ();
        }
    xSemaphoreGive (getLwIpMutex ());
}

tcpConnection_t::~tcpConnection_t () { close (); }

// recv with traffic reccording
int tcpConnection_t::recv (void *buf, size_t len) {
    int received = -1;

    while (received < 0) { // read blocks of incoming data
        xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            received = ::recv (__connectionSocket__, (char *) buf, len, 0);
        xSemaphoreGive (getLwIpMutex ());

        if (received <= 0)
            switch (errno) {
                case 107:   // ENOTCONN (all the sockets are non-blocking)
                // case 119:   // EALREADY (all the sockets are non-blocking)
                case  11:   // EAGAIN or EWOULDBLOCK
                            if (idleTimeout ()) {
                                cout << ( dmesgQueue << "[tcpConn] idle timeout" ) << endl;
                                return -1;
                            } else {
                                // continue waiting
                                delay (25);
                                continue;
                            }
                case   0:   // connection closed by peer
                            cout << ( dmesgQueue << "[tcpConn] " << "connection closed by peer" ) << endl;
                            return 0;
                case 128:   // ENOTSOCK (or the client closed the connection), don't log
                            return -1;
                default:
                            cout << ( dmesgQueue << "[tcpConn] " << "error: " << errno << " " << strerror (errno) ) << endl;
                            return -1;
            }
    }

    stillActive ();
    
    // since no semaphore is used here network traffic logging may not be completely accurate in multitaskin environment
    networkTraffic ().bytesReceived += received;
    networkTraffic () [__connectionSocket__].bytesReceived += received;

    return received;
}

// reads the whole block of bytes
// returns len if OK
//           0 if the peer closed the connection
//          -1 if error occured
int tcpConnection_t::recvBlock (void *buf, size_t len) {
    int receivedTotal = 0;
    while (receivedTotal < (int) len) {
        int receivedThisTime = recv ((char *) buf + receivedTotal, len - receivedTotal);
        if (receivedThisTime <= 0) {
            return receivedThisTime;
        }
        receivedTotal += receivedThisTime;
    }
    return receivedTotal;
}


// reads and fills the buffer until endingString is read (also finishes the string with ending 0)
// returns the number of characters read up to len - 1 if OK
//                                             len if the buffer is too small
//                                             0 if the peer closed the connection
//                                             -1 if error occured
int tcpConnection_t::recvString (char *buf, size_t len, const char *endingString) {
    int receivedTotal = 0;
    int receivedThisTime;

    while (receivedTotal != len - 1) { // read blocks of incoming data
        xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            receivedThisTime = ::recv (__connectionSocket__, buf + receivedTotal, len - receivedTotal - 1, 0);
        xSemaphoreGive (getLwIpMutex ());
        if (receivedThisTime <= 0)
            switch (errno) {
                case 107:   // ENOTCONN (all the sockets are non-blocking)
                // case 119:   // EALREADY (all the sockets are non-blocking)
                case  11:   // EAGAIN or EWOULDBLOCK
                            if (idleTimeout ()) {
                                cout << ( dmesgQueue << "[tcpConn] " << "timeout" ) << endl;
                                return -1;
                            } else {
                                // continue waiting
                                delay (25);
                                continue;
                            }
                case   0:   // connection closed by peer
                            cout << ( dmesgQueue << "[tcpConn] " << "connection closed by peer" ) << endl;
                            return 0;
                case 128:   // ENOTSOCK (or the client closed the connection), don't log
                            return -1;
                default:
                            cout << ( dmesgQueue << "[tcpConn] " << "error: " << errno << " " << strerror (errno) ) << endl;
                            return -1;
            }

        stillActive ();
        receivedTotal += receivedThisTime;
        
        // since no semaphore is used here network traffic logging may not be completely accurate in multitaskin environment
        networkTraffic ().bytesReceived += receivedThisTime;
        networkTraffic () [__connectionSocket__].bytesReceived += receivedThisTime;

        // the following code assumes that the other side sends command or reply (according to the protocol) that ends with endingString
        buf [receivedTotal] = 0;
        if (strstr (buf, endingString))
            return receivedTotal;
    }

    // we read the whole buffer up to len-1 but the ending string did not arrive - buffer is too small
    return len; 
}

// checks (but not actually reads) if bytes are pending to be read
// returns len if yes (also fills the buffer)
//           0 if no 
//          -1 if error occured (including the case if the peer closed the connection)
int tcpConnection_t::peek (void *buf, size_t len) { 
    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
        int received = ::recv (__connectionSocket__, (char *) buf, len, MSG_PEEK);
    xSemaphoreGive (getLwIpMutex ());
    if (received <= 0)
        switch (errno) {
            case 107:   // ENOTCONN (all the sockets are non-blocking)
            // case 119:   // EALREADY (all the sockets are non-blocking)
            case  11:   // EAGAIN or EWOULDBLOCK
                        if (idleTimeout ()) {
                            cout << ( dmesgQueue << "[tcpConn] " << "timeout" ) << endl;
                            return -1;
                        } else {
                            return 0;
                        }
            case   0:   // connection closed by peer
                        cout << ( dmesgQueue << "[tcpConn] " << "connection closed by peer" ) << endl;
                        return -1;
            case 128:   // ENOTSOCK (or the client closed the connection), don't log
                        return -1;
            default:
                        cout << ( dmesgQueue << "[tcpConn] " << "error: " << errno << " " << strerror (errno) ) << endl;
                        return -1;
        }

    stillActive ();

    return received;
}

// sends the whole block of charcters
// returns len in case of OK
//           0 if the peer closed the connection
//          -1 in case of error
int tcpConnection_t::sendBlock (void *buf, size_t len) {
    constexpr size_t MAX_BLOCK_SIZE = 1440;
    
    size_t sentTotal = 0;
    while (sentTotal < len) {
        size_t n = min (MAX_BLOCK_SIZE, len - sentTotal);
        xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            int sentThisTime = ::send (__connectionSocket__, (char *) buf + sentTotal, n, 0);    
        xSemaphoreGive (getLwIpMutex ());

        if (sentThisTime <= 0)
            switch (errno) {
                case 107:   // ENOTCONN (all the sockets are non-blocking)
                // case 119:   // EALREADY (all the sockets are non-blocking)
                case  11:   // EAGAIN or EWOULDBLOCK
                            if (idleTimeout ()) {
                                cout << ( dmesgQueue << "[tcpConn] " << "timeout" ) << endl;
                                return -1;
                            } else {
                                // continue waiting
                                delay (25);
                                continue;
                            }
                case   0:   // connection closed by peer
                            cout << ( dmesgQueue << "[tcpConn] " << "connection closed by peer" ) << endl;  
                            return 0;
                case 128:   // ENOTSOCK (or the client closed the connection), don't log
                            return -1;
                default:
                            cout << ( dmesgQueue << "[tcpConn] " << "error: " << errno << " " << strerror (errno) ) << endl;
                            return -1;
            }

        stillActive ();
        sentTotal += sentThisTime;

        networkTraffic ().bytesSent += sentThisTime;
        networkTraffic () [__connectionSocket__].bytesSent += sentThisTime;

        if (sentTotal < len) 
            delay (25);
    } 

    return sentTotal;
}

// sends the whole string of charcteers
// returns the number of characters sent in case of OK
//                                     0 if the peer closed the connection
//                                    -1 in case of error
int tcpConnection_t::sendString (const char *buf) {
    return sendBlock ((void *) buf, strlen (buf));
}


void tcpConnection_t::close () {
    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
        if (__connectionSocket__ != -1) {
            ::close (__connectionSocket__);
            __connectionSocket__ = -1;
            // networkTraffic () [__connectionSocket__] = {0, 0};            
        }
    xSemaphoreGive (getLwIpMutex ());
}
