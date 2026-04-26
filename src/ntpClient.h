/*

  ntpClient.h

  This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


  This library is based on Let's make a NTP Client in C: https://lettier.github.io/posts/2016-04-26-lets-make-a-ntp-client-in-c.html
  which I'm keeping here as close to the original as possible due to its comprehensive explanation.

  April 27, 2026, Bojan Jurca

*/


#pragma once
#ifndef __NTP_CLIENT__
  #define __NTP_CLIENT__


  #include <WiFi.h>
  #include <lwip/netdb.h>
  #include <time.h>
  #include <LwIpMutex.h>
  #include <gai_strerror.h>
  #include <string.h>
  #include <dmesg.hpp>
  #include <ostream.hpp>


  // TUNING PARAMETER

  #define MAX_ETC_NTP_CONF_SIZE (1 * 1024)                // 1 KB will usually do - initialization reads the whole /etc/ntp.conf file in the memory 


  class ntpClient_t {

    private:

      // Meyers singleton - 
      inline char (&__getNtpServer__ ()) [3][255] {
          static char ntpServer [3][255] = {}; // DNS host name may have max 253 characters
          return ntpServer;
      }

      // Meyers singleton
      inline time_t& __getStartUpTime__ () {
          static time_t startUpTime = 0;
          return startUpTime;
      }


    public:
      ntpClient_t ();
      ntpClient_t (const char *ntpServer0,
                   const char *ntpServer1 = NULL,
                   const char *ntpServer2 = NULL);

      #ifdef __THREAD_SAFE_FS__

          ntpClient_t (threadSafeFS::FS& fileSystem,
                       const char *ntpServer0 = NULL,
                       const char *ntpServer1 = NULL,
                       const char *ntpServer2 = NULL) {

              // create /etc/crontb file if it doesn't exist
              if (!fileSystem.isFile ("/etc/ntp.conf")) {
                  if (!fileSystem.isDirectory ("/etc"))
                      fileSystem.mkdir ("/etc");
                  cout << ( dmesgQueue << "[NTP] " "creating default /etc/ntp.conf" );
                  bool created = false;
                  threadSafeFS::File f = fileSystem.open ("/etc/ntp.conf", "w");
                  if (f) {
                                    created = fprintf (f, "# configuration for NTP - reboot for changes to take effect\r\n\r\n"
                                                          "server1 ");
                      if (created)  created = fprintf (f, ntpServer0 ? ntpServer0 : "");
                      if (created)  created = fprintf (f, "\r\n"
                                                          "server2 ");
                      if (created)  created = fprintf (f, ntpServer1 ? ntpServer1 : "");                                                          
                      if (created)  created = fprintf (f, "\r\n"
                                                          "server3 ");
                      if (created)  created = fprintf (f, ntpServer2 ? ntpServer2 : "");  
                      if (created)  created = fprintf (f, "\r\n");
                      f.close ();
                  }
                  if (!created)
                      cout << ( dmesgQueue << "[NTP] " "default /etc/ntp.conf could not be created" );
              }

              // read /etc/ntp.conf file
              char buffer [MAX_ETC_NTP_CONF_SIZE] = "\n";
              if (fileSystem.readConfiguration (buffer + 1, sizeof (buffer) - 3, "/etc/ntp.conf")) {
                  strcat (buffer, "\n");
                  __getNtpServer__ () [0][0] = __getNtpServer__ () [1][0] = __getNtpServer__ () [2][0] = 0;
                  char *p;                    
                  if ((p = strstr (buffer, "\nserver1"))) sscanf (p + 8, "%*[ =]%253[0-9A-Za-z.-]", __getNtpServer__ () [0]);
                  if ((p = strstr (buffer, "\nserver2"))) sscanf (p + 8, "%*[ =]%253[0-9A-Za-z.-]", __getNtpServer__ () [1]);
                  if ((p = strstr (buffer, "\nserver3"))) sscanf (p + 8, "%*[ =]%253[0-9A-Za-z.-]", __getNtpServer__ () [2]);
              } else {
                  cout << ( dmesgQueue << "[NTP] " "error reading /etc/ntp.conf" );
              }
          }
      #endif

      const char *syncTime ();                          // sync using all servers
      const char *syncTime (int ntpServerIndex);        // sync using index
      const char *syncTime (const char *ntpServerName); // sync using name

      const char *setTime (const time_t newTime);       // set the time

      time_t getUpTime () { 
        time_t t = time (NULL);
        if (__getStartUpTime__ () <= t)
            return t - __getStartUpTime__ ();
        else
          return millis () / 1000; // wrong time settings, this would probably be the best answer
      }

  };

#endif
