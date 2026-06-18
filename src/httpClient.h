/*

    httpClient.h 
  
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


#ifndef __HTTP_CLIENT_H__
    #define __HTTP_CLIENT_H__


    #include <WiFi.h>
    #include "tcpConnection.h"
    #include <Cstring.hpp>      // include LightweightSTL library: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino


    // ----- functions and variables in this modul -----

    String httpRequest (const char *httpServer, int httpPort, const char *httpRequest, const char *httpMethod, unsigned long timeOut);


    // ----- TUNNING PARAMETERS -----

    #ifndef HTTP_REPLY_TIME_OUT
        #define HTTP_REPLY_TIME_OUT 3                 // 3 s
    #endif
    #ifndef HTTP_REPLY_BUFFER_SIZE
        #define HTTP_REPLY_BUFFER_SIZE 1440
    #endif


    // ----- CODE -----

    inline String httpRequest (const char *httpServer, int httpPort = 80, const char *httpAddress = "/", const char *httpMethod = "GET", unsigned long timeOut = HTTP_REPLY_TIME_OUT) {

        if (!WiFi.isConnected () || WiFi.localIP () == IPAddress (0, 0, 0, 0))
            return "not connected to WiFi";

        tcpConnection_t httpClient (httpServer, httpPort);
        if (*httpClient.errText ())
            return httpClient.errText ();

        httpClient.setIdleTimeout (HTTP_REPLY_TIME_OUT);

        // 1. send HTTP request
        Cstring<300> httpRequest;
        httpRequest += httpMethod;
        httpRequest += " ";
        httpRequest += httpAddress;
        httpRequest += " HTTP/1.0\r\nHost: ";
        httpRequest += httpServer;
        httpRequest += "\r\n\r\n"; // 1.0 HTTP does not know keep-alive directive - we want the server to close the connection immediatelly after sending the reply
        if (httpRequest.errorFlags ())
            return "HTTP request too long";

        switch (httpClient.sendString (httpRequest)) {
            case -1:  return strerror (errno);
            case 0:   return "connection closed by peer";
            default:  break; // OK
        }

        // 2. read HTTP reply
        String httpReply ("");
        char buffer [HTTP_REPLY_BUFFER_SIZE];
        int receivedThisTime;

        while (true) { // read blocks of incoming data
            receivedThisTime = httpClient.recv (buffer, HTTP_REPLY_BUFFER_SIZE - 1);
            switch (receivedThisTime) {
                case -1:    // error
                            if (errno == 128) // ENOTSOCK (or the client closed the connection)
                                if (httpReply != "")
                                    return httpReply; // we could have got HTTP reply but the COntent-Length is not reported correctly
                            return strerror (errno);
                case 0:     // connection closed by peer
                            if (httpReply != "")
                                return httpReply; // we could have got HTTP reply but the COntent-Length is not reported correctly
                            return "connection closed by peer";
                default:      // block arrived
                                buffer [receivedThisTime] = 0;
                                if (!httpReply.concat (buffer))
                                    return "Out of memory";

                                // check if HTTP reply is complete
                                char *p = strstr (httpReply.c_str (), "\nContent-Length:");
                                if (p) {
                                    p += 16;
                                    unsigned int contentLength;
                                    if (sscanf (p, "%u", &contentLength) == 1) {
                                        p = strstr (p, "\r\n\r\n"); // the content comes afterwards
                                        if (p && contentLength == strlen (p + 4))
                                            return httpReply;
                                    }
                                }
                                // else continue reading
                                break;
            } // switch
        } // while       
        return ""; // never executes
    }

#endif
