/*

    httpClient.hpp 
  
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  

    January 1, 2026, Bojan Jurca


    Classes used in this module:

        tcpClient_t

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


#ifndef __HTTP_CLIENT__
    #define __HTTP_CLIENT__


    #include <WiFi.h>
    #include "tcpClient.h"
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

    inline String httpRequest (const char *httpServer, int httpPort, const char *httpAddress, const char *httpMethod = (const char *) "GET", unsigned long timeOut = HTTP_REPLY_TIME_OUT) {

        if (!WiFi.isConnected () || WiFi.localIP () == IPAddress (0, 0, 0, 0))
            return "not connected";

        tcpClient_t httpClient (httpServer, httpPort);
        if (httpClient.errText ())
            return httpClient.errText ();

        httpClient.setIdleTimeout (HTTP_REPLY_TIME_OUT);

        // 1. send HTTP request
        cstring httpRequest;
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
            case 0:   return (const char *) "Connection closed by peer";
            default:  break; // OK
        }

        // 2. read HTTP reply
        String httpReply ("");
        char buffer [HTTP_REPLY_BUFFER_SIZE];
        int receivedTotal = 0;
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
                            return (const char *) "Connection closed by peer";
                default:      // block arrived
                                buffer [receivedThisTime] = 0;
                                if (!httpReply.concat (buffer))
                                    return (const char *) "Out of memory";

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
