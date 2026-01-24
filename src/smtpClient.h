/*

    smtpClient.hpp 
 
    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library

  
    January 1, 2026, Bojan Jurca

*/


#ifndef __SMTP_CLIENT__
    #define __SMTP_CLIENT__


    #include <WiFi.h>
    #include "tcpClient.h"
    #include <mbedtls/base64.h>
    #include <Cstring.hpp>      // include LightweightSTL library: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
    #include <dmesg.hpp>
    #include <ostream.hpp>  // use serial console if dmesg.hhp is not #included


    // ----- functions and variables in this modul -----


    // TUNNING PARAMETERS

    #define SMTP_TIME_OUT 3                         // 3 sec 
    #define SMTP_BUFFER_SIZE 256                    // for constructing SMTP commands and reading SMTP reply
    #ifndef HOSTNAME
        #define HOSTNAME "Esp32Server"              // use default if not defined previously
    #endif
    #define MAX_ETC_MAIL_SENDMAIL_CF 1 * 1024       // 1 KB will usually do - sendmail reads the whole /etc/mail/sendmail.cf file in the memory 


    // ----- CODE -----


    // sends message, returns error or success text (from SMTP server)
    Cstring<300> sendMail (const char *message, const char *subject, const char *to, const char *from, const char *password, const char *userName, int smtpPort, const char *smtpServer);


    #ifdef __THREAD_SAFE_FS__
        // sends message, returns error or success text, fills empty parameters with the ones from configuration file /etc/mail/sendmail.cf
        inline Cstring<300> sendMail (threadSafeFS::FS& fileSystem, const char *message = "", const char *subject = "", const char *to = "", const char *from = "", const char *password = "", const char *userName = "", int smtpPort = 0, const char *smtpServer = "") {
            char buffer [MAX_ETC_MAIL_SENDMAIL_CF + 1];
            strcpy (buffer, "\n");
            if (fileSystem.readConfiguration (buffer + 1, sizeof (buffer) - 3, "/etc/mail/sendmail.cf")) {
                strcat (buffer, "\n");                    
                char *p = NULL;
                char *q;
                if (!*smtpServer) if ((smtpServer = strstr (buffer, "\nsmtpServer ")))  { smtpServer += 12; }
                if (!smtpPort)    if ((p = strstr (buffer, "\nsmtpPort ")))             { p += 10; }
                if (!*userName)   if ((userName = strstr (buffer, "\nuserName ")))      { userName += 10; }
                if (!*password)   if ((password = strstr (buffer, "\npassword ")))      { password += 10; }
                if (!*from)       if ((from = strstr (buffer, "\nfrom ")))              { from += 6; }
                if (!*to)         if ((to = strstr (buffer, "\nto ")))                  { to += 4; }          
                if (!*subject)    if ((subject = strstr (buffer, "\nsubject ")))        { subject += 9; }
                if (!*message)    if ((message = strstr (buffer, "\nmessage ")))        { message += 9; }
                for (q = buffer; *q; q++)
                    if (*q < ' ') *q = 0;
                if (p) smtpPort = atoi (p);
            }

            if (!*to || !*from || !*password || !*userName || !smtpPort || !*smtpServer) {
                cout << ( dmesgQueue << "[smtpClient] not all the arguments are set in /etc/mail/sendmail.cf" );
                return "[smtpClient] not all the arguments are set in /etc/mail/sendmail.cf";
            } else {
                
                Serial.printf ("message=%s\n", message);
                Serial.printf ("subject=%s\n", subject);
                Serial.printf ("to=%s\n", to);
                Serial.printf ("from=%s\n", from);
                Serial.printf ("password=%s\n", password);
                Serial.printf ("userName=%s\n", userName);
                Serial.printf ("smtpPort=%i\n", smtpPort);
                Serial.printf ("smtpServer=%s\n", smtpServer);
                
                return sendMail (message, subject, to, from, password, userName, smtpPort, smtpServer); 
            }
        }
    #endif

#endif
