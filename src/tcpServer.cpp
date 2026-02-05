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


#include <WiFi.h>
#include "tcpServer.h"
#include <dmesg.hpp>
#include <ostream.hpp>


int __runningTcpConnections__ = 0;

tcpServer_t::tcpServer_t (int serverPort,
                          bool (*firewallCallback) (char *clientIP, char *serverIP),
                          bool runListenerInItsOwnTask) : __serverPort__ (serverPort), 
                                                          __firewallCallback__ (firewallCallback),
                                                          __runListenerInItsOwnTask__ (runListenerInItsOwnTask) {
  
  xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);

    // create listening socket
      __listeningSocket__ = socket (AF_INET6, SOCK_STREAM, 0);
    if (__listeningSocket__ == -1) {
      cout << ( dmesgQueue << "[tcpServer] " << "socket error: " << errno << " " << strerror (errno) );
      xSemaphoreGive (getLwIpMutex ());
      return;
    }

    // allow both IPv4 and IPv6 connections
    int opt = 0;
    if (setsockopt (__listeningSocket__, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof (opt)) < 0) {
      cout << ( dmesgQueue << "[tcpServer] " << "setsockopt error: " << errno << " " << strerror (errno) );
      close (__listeningSocket__);
      __listeningSocket__ = -1;
      xSemaphoreGive (getLwIpMutex ());      
      return;
    }

    // make address reusable
    int flag = 1;
    if (setsockopt (__listeningSocket__, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag)) == -1) {
      cout << ( dmesgQueue << "[tcpServer] " << "setsockopt error: " << errno << " " << strerror (errno) );
      close (__listeningSocket__);
      __listeningSocket__ = -1;
      xSemaphoreGive (getLwIpMutex ());
      return;
    }

    // bind listening socket
    struct sockaddr_in6 serverAddress = {};
    serverAddress.sin6_family = AF_INET6;
    serverAddress.sin6_port = htons (__serverPort__);

    if (bind (__listeningSocket__, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
      cout << ( dmesgQueue << "[tcpServer] " << "bind error: " << errno << " " << strerror (errno) );
      close (__listeningSocket__);
      __listeningSocket__ = -1;
      xSemaphoreGive (getLwIpMutex ());
      return;
    }

    // make socket a listening socket
    if (listen (__listeningSocket__, 4) == -1) {
      cout << ( dmesgQueue << "[tcpServer] " << "listen error: " << errno << " " << strerror (errno) );
      __listeningSocket__ = -1;
      xSemaphoreGive (getLwIpMutex ());
      return;
    }

    // make listening socket non-blocking
    if (fcntl (__listeningSocket__, F_SETFL, O_NONBLOCK) < 0) {
      cout << ( dmesgQueue << "[tcpServer] " << "fcntl error: " << errno << " " << strerror (errno) );
      __listeningSocket__ = -1;
      xSemaphoreGive (getLwIpMutex ());        
      return;
    }

  xSemaphoreGive (getLwIpMutex ());


  __state__ = RUNNING;


  // start listener task if needed
  if (runListenerInItsOwnTask) {
    #define tskNORMAL_PRIORITY (tskIDLE_PRIORITY + 1)
    BaseType_t taskCreated = xTaskCreate ([] (void *thisInstance) {
      tcpServer_t *ths = (tcpServer_t *) thisInstance;
      cout << ( dmesgQueue << "[tcpServer] " << "listener on port " << ths->__serverPort__ << " started on core " << xPortGetCoreID () );

      while (ths->__listeningSocket__ > -1) {
        delay (25);
        ths->accept ();

        static UBaseType_t lastHighWaterMark = TCP_LISTENER_STACK_SIZE;
        UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
        if (lastHighWaterMark > highWaterMark) {
          cout << ( dmesgQueue << "[tcpServer] " << "new listener's stack high water mark: " << highWaterMark << " bytes not used" );
          lastHighWaterMark = highWaterMark;
        }
      }

      cout << ( dmesgQueue << "[tcpServer] " << "on port " << ths->__serverPort__ << " stopped" );

      xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
        if (ths->__listeningSocket__ != -1) {
          close (ths->__listeningSocket__);
          ths->__listeningSocket__ = -1;
        }
      xSemaphoreGive (getLwIpMutex ());

      ths->__state__ = NOT_RUNNING;
      vTaskDelete (NULL);
    }, "tcpListener", TCP_LISTENER_STACK_SIZE, this, tskNORMAL_PRIORITY, NULL);

    if (pdPASS != taskCreated) {
      __state__ = NOT_RUNNING;
      cout << ( dmesgQueue << "[tcpServer] " << "xTaskCreate error" );
      xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
        if (__listeningSocket__ != -1) {
          close (__listeningSocket__);
          __listeningSocket__ = -1;
        }
      xSemaphoreGive (getLwIpMutex ());
    }
  } 

  return;
}

tcpServer_t::~tcpServer_t () {
  xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
    if (__listeningSocket__ != -1) {
      close (__listeningSocket__);
      __listeningSocket__ = -1;
    }
  xSemaphoreGive (getLwIpMutex ());


  // wait until listener task finishes before unloading so that variables are still in the memory while it is running
  if (__runListenerInItsOwnTask__) {
    while (__state__ != NOT_RUNNING)
      delay (25);
  }
}

tcpConnection_t *tcpServer_t::accept () {
  int connectionSocket;
  char clientIP [INET6_ADDRSTRLEN];
  struct sockaddr_storage connectingAddress;
  socklen_t connectingAddressSize = sizeof (connectingAddress);


    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
      if (__listeningSocket__ == -1) {
        xSemaphoreGive (getLwIpMutex ());
        return NULL;
      }

      connectionSocket = ::accept (__listeningSocket__, (struct sockaddr *) &connectingAddress, &connectingAddressSize);
      if (connectionSocket == -1) {

        if (errno == EAGAIN || errno == ENAVAIL) {
          ; // this is not an error it's just that no connection arrived
        } else {
          cout << ( dmesgQueue << "[tcpServer] " << "accept error: " << errno << " " << strerror (errno) );
        }      
        xSemaphoreGive (getLwIpMutex ());
        return NULL;
      }

      // set socket time-out (without error checking, this is just a back-up option)
      struct timeval tv = { SOCKET_TIMEOUT, 0 };
      setsockopt (connectionSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof (tv));
      setsockopt (connectionSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *) &tv, sizeof (tv));
      
    xSemaphoreGive (getLwIpMutex ());

  // as long as only one task accesses the socket the follwing LwIP functions can be used withot semaphore

  // get client's IP
  if (connectingAddress.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *) &connectingAddress;
    inet_ntop (AF_INET, &s->sin_addr, clientIP, sizeof (clientIP));
  } else {
    struct sockaddr_in6 *s = (struct sockaddr_in6 *) &connectingAddress;
    inet_ntop (AF_INET6, &s->sin6_addr, clientIP, sizeof (clientIP));
  }

  // get server's IP
  char serverIP [INET6_ADDRSTRLEN] = "";
  struct sockaddr_storage thisAddress = {};
  socklen_t len = sizeof (thisAddress);
  if (getsockname (connectionSocket, (struct sockaddr *) &thisAddress, &len) != -1) {
    struct sockaddr_in6 *s = (struct sockaddr_in6 *) &thisAddress;
    inet_ntop (AF_INET6, &s->sin6_addr, serverIP, sizeof (serverIP));
    if (connectingAddress.ss_family == AF_INET) {
      char *p = serverIP;
      for (char *q = p; *q; q++)
        if (*q == ':')
          p = q;
      if (p != serverIP)
        strcpy (serverIP, p + 1);
    }
  }

  // check firewall
  if (__firewallCallback__ && !__firewallCallback__ (clientIP, serverIP)) {
    cout << ( dmesgQueue << "[tcpServer] " << "firewall rejected connection from " << clientIP << " to " << serverIP );
    close (connectionSocket);
    return NULL;
  }

  return __createConnectionInstance__ (connectionSocket, clientIP, serverIP);
}

tcpConnection_t *tcpServer_t::__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) {
    return new (std::nothrow) tcpConnection_t (connectionSocket, clientIP, serverIP);
}
