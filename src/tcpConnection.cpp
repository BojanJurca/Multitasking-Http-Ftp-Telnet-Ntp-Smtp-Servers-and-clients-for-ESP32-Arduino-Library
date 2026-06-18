/*

    tcpConnection.cpp 
  
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


#include "tcpConnection.h"
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dmesg.hpp>
#include <ostream.hpp>


tcpConnection_t::tcpConnection_t () {
    stillActive ();
}

// server connection constructor
tcpConnection_t::tcpConnection_t (int connectionSocket, const char *clientIP, const char *serverIP) {
    __connectionSocket__ = connectionSocket;
    strncpy (__clientIP__, clientIP, sizeof (__clientIP__) - 1);
    strncpy (__serverIP__, serverIP, sizeof (__serverIP__) - 1);
    networkTraffic () [__connectionSocket__] = {0, 0};

    // make connection socket non-blocking
    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);    
        if (fcntl (__connectionSocket__, F_SETFL, O_NONBLOCK) < 0) {
            cout << ( dmesgQueue << "[tcpConn] " << "error: " << errno << " " << strerror (errno) );
            xSemaphoreGive (getLwIpMutex ());
            close ();
        }
    xSemaphoreGive (getLwIpMutex ());

    stillActive ();
}

// client connection constructor
tcpConnection_t::tcpConnection_t (const char *serverName, int serverPort) : tcpConnection_t () {

    if (!WiFi.isConnected () || WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // esp32 can crash without this check
      __errText__ = "not connected to WiFi";
     	cout << ( dmesgQueue << ("[tcpConn] " "not connected to WiFi") );
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
      cout << ( dmesgQueue << "[tcpConn] " << __errText__ );
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
        cout << ( dmesgQueue << "[tcpConn] " << __errText__ );
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
        cout << ( dmesgQueue << "[tcpConn] " << __errText__ );
        ::close (__connectionSocket__);
        __connectionSocket__ = -1;
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
          cout << ( dmesgQueue << "[tcpConn] " << __errText__ << " " << __serverIP__ );
        } else {
          server_addr.sin6_port = htons (serverPort);
          if (connect (__connectionSocket__, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
            if (errno != EINPROGRESS) {
              __errText__ = strerror (errno);
              cout << ( dmesgQueue << "[tcpConn] " << __errText__ );
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
          cout << ( dmesgQueue << "[tcpConn] " << __errText__ << " " << __serverIP__ );
        } else {
          server_addr.sin_port = htons (serverPort);
          if (connect (__connectionSocket__, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
            if (errno != EINPROGRESS) {
              __errText__ = strerror (errno);
              cout << ( dmesgQueue << "[tcpConn] " << __errText__ );
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
        cout << ( dmesgQueue << "[tcpConn] " << __errText__ );
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
                    cout << ( dmesgQueue << "[tcpConn] " << __errText__ );
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

tcpConnection_t::~tcpConnection_t () { 
  close (); 
}

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
                            [[fallthrough]];
                // case 119:   // EALREADY (all the sockets are non-blocking)
                case  11:   // EAGAIN or EWOULDBLOCK
                            if (idleTimeout ()) {
                                // cout << ( dmesgQueue << "[tcpConn] idle timeout" ); // do not log, this is a normal end of connection in httpServer
                                __errText__ = "time-out";
                                return -1;
                            } else {
                                // continue waiting
                                delay (25);
                                continue;
                            }
                case   0:   // connection closed by peer
                            __errText__ = "connection closed by peer";
                            cout << ( dmesgQueue << "[tcpConn] " << "connection closed by peer" );
                            return 0;
                case 104:   // Connection reset by peer, don't log
                            [[fallthrough]];
                case 128:   // ENOTSOCK (or the client closed the connection), don't log
                            __errText__ = "client closed the connection";
                            return -1;
                default:
                            __errText__ = strerror (errno);
                            cout << ( dmesgQueue << "[tcpConn] " << "error: " << errno << " " << __errText__ );
                            return -1;
            }
      __errText__ = "";
    }

    stillActive ();
    
    // since no semaphore is used here network traffic logging may not be completely accurate in multitaskin environment
    networkTraffic ().bytesReceived += received;
    networkTraffic () [__connectionSocket__].bytesReceived += received;

    return received;
}

// send with traffic reccording
int tcpConnection_t::send (void *buf, size_t len) {
    int sent = -1;
    while (sent < 0) {
        xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            sent = ::send (__connectionSocket__, buf, len, 0);    
        xSemaphoreGive (getLwIpMutex ());

        if (sent <= 0)
            switch (errno) {
                case 107:   // ENOTCONN (all the sockets are non-blocking)
                            [[fallthrough]];
                // case 119:   // EALREADY (all the sockets are non-blocking)
                case  11:   // EAGAIN or EWOULDBLOCK
                            if (idleTimeout ()) {
                                // cout << ( dmesgQueue << "[tcpConn] " << "timeout" ); // do not log, this is a normal end of connection in httpServer
                                __errText__ = "time-out";
                                return -1;
                            } else {
                                // continue waiting
                                delay (25);
                                continue;
                            }
                case   0:   // connection closed by peer
                            __errText__ = "connection closed by peer";
                            cout << ( dmesgQueue << "[tcpConn] " << "connection closed by peer" );  
                            return 0;
                case 104:   // Connection reset by peer, don't log
                            [[fallthrough]];
                case 128:   // ENOTSOCK (or the client closed the connection), don't log
                            __errText__ = "client closed the connection";
                            return -1;
                default:
                            __errText__ = strerror (errno);
                            cout << ( dmesgQueue << "[tcpConn] " << "error: " << errno << " " << __errText__ );
                            return -1;
            }
        __errText__ = "";

        stillActive ();

        networkTraffic ().bytesSent += sent;
        networkTraffic () [__connectionSocket__].bytesSent += sent;
    } 

    return sent;
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
        receivedThisTime = recv (buf + receivedTotal, len - receivedTotal - 1);
        if (receivedThisTime <= 0)
            return receivedThisTime;

        receivedTotal += receivedThisTime;

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
                        [[fallthrough]];
            // case 119:   // EALREADY (all the sockets are non-blocking)
            case  11:   // EAGAIN or EWOULDBLOCK
                        if (idleTimeout ()) {
                            // cout << ( dmesgQueue << "[tcpConn] " << "timeout" ); // do not log, this is a normal end of connection in httpServer
                            __errText__ = "time-out";
                            return -1;
                        } else {
                            return 0;
                        }
            case   0:   // connection closed by peer
                        __errText__ = "connection closed by peer";
                        cout << ( dmesgQueue << "[tcpConn] " << "connection closed by peer" );
                        return -1;
            case 104:   // Connection reset by peer, don't log
                        [[fallthrough]];
            case 128:   // ENOTSOCK (or the client closed the connection), don't log
                        __errText__ = "client closed the connection";
                        return -1;
            default:
                        __errText__ = strerror (errno);
                        cout << ( dmesgQueue << "[tcpConn] " << "error: " << errno << " " << __errText__ );
                        return -1;
        }
    __errText__ = "";

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
        int sentThisTime = send ((char *) buf + sentTotal, n);    

        if (sentThisTime <= 0)
            return sentThisTime;
    
        sentTotal += sentThisTime;

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
