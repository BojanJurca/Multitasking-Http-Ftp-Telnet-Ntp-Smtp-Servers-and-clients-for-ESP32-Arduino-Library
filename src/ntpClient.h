/*

  ntpClient.h

  This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library

  This library is based on Let's make a NTP Client in C: https://lettier.github.io/posts/2016-04-26-lets-make-a-ntp-client-in-c.html
  which I'm keeping here as close to the original as possible due to its comprehensive explanation.

  January 1, 2026, Bojan Jurca

*/


#pragma once
#ifndef __NTP_CLIENT__
  #define __NTP_CLIENT__


  #include <WiFi.h>
  #include <lwip/netdb.h>
  #include <time.h>
  #include <LwIpMutex.h>


  // missing function in LwIP
  static const char *gai_strerror (int err);


  class ntpClient_t {

    private:
      char __ntpServer__ [3][255] = {};  // DNS host name may have max 253 characters

    public:
      ntpClient_t (const char *ntpServer0,
                   const char *ntpServer1 = NULL,
                   const char *ntpServer2 = NULL);

      const char *syncTime ();                          // sync using all servers
      const char *syncTime (int ntpServerIndex);        // sync using index
      const char *syncTime (const char *ntpServerName); // sync using name

  };

#endif
