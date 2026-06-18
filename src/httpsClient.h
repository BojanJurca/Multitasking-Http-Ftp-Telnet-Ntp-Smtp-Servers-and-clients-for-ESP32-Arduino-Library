/*

    httpsClient.h
  
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
#ifndef __HTTPS_CLIENT_H__
    #define __HTTPS_CLIENT_H__


    #include <Cstring.hpp>
    #include "tlsConnection.h" // uses WolfSSL library


    // ----- functions and variables in this modul -----

    String httpsRequest (const char *httpServer, int httpPort, const char *httpRequest, const char *httpMethod, unsigned long timeOut);


    // ----- TUNNING PARAMETERS -----

    #ifndef HTTPS_REPLY_TIME_OUT
        #define HTTPS_REPLY_TIME_OUT 3                 // 3s s
    #endif
    #ifndef HTTPS_REPLY_BUFFER_SIZE
        #define HTTPS_REPLY_BUFFER_SIZE 1440
    #endif


    // ----- CODE -----

    inline String httpsRequest (const char *httpsServer, int httpsPort = 443, const char *httpsAddress = "/", const char *httpsMethod = "GET", unsigned long timeOut = HTTPS_REPLY_TIME_OUT) {
        if (!WiFi.isConnected () || WiFi.localIP () == IPAddress (0, 0, 0, 0))
            return "not connected to WiFi";

        // --- WolfSSL init ---
        tlsSystem.Init (); // wolfSSL_Init ();

        // WolfSSL need more stack memory that Arduino normaly provides so run
        // the rest of the code in a separate task and wait for it to finish
        struct params_t {
            const char *httpsServer;
            int httpsPort;
            const char *httpsAddress;
            const char *httpsMethod;
            unsigned long timeOut;
            SemaphoreHandle_t done;
            String httpsReply;
        } params = { httpsServer, httpsPort, httpsAddress, httpsMethod, timeOut, xSemaphoreCreateBinary (), "" };
        if (pdPASS == xTaskCreate ([] (void *ptr) {
                                                    params_t *params = static_cast<params_t*>(ptr);

                                                    tlsConnection_t tlsConnection (params->httpsServer, params->httpsPort, params->timeOut);
                                                    if (*tlsConnection.errText ()) {
                                                        params->httpsReply = tlsConnection.errText ();
                                                        xSemaphoreGive (params->done);
                                                        vTaskDelete (NULL);
                                                    }

                                                    // 1. send HTTP request
                                                    Cstring<300> httpsRequest;
                                                    httpsRequest += params->httpsMethod;
                                                    httpsRequest += " ";
                                                    httpsRequest += params->httpsAddress;
                                                    httpsRequest += " HTTP/1.0\r\nHost: ";
                                                    httpsRequest += params->httpsServer;
                                                    httpsRequest += "\r\n\r\n"; // 1.0 HTTP does not know keep-alive directive - we want the server to close the connection immediatelly after sending the reply
                                                    if (httpsRequest.errorFlags ()) {
                                                        params->httpsReply = "HTTP request too long";
                                                        xSemaphoreGive (params->done);
                                                        vTaskDelete (NULL);                                                        
                                                    }

                                                    int sent = tlsConnection.sendString (httpsRequest);
                                                    if (sent <= 0) {
                                                        params->httpsReply = "TLS write error";
                                                        xSemaphoreGive (params->done);
                                                        vTaskDelete (NULL);    
                                                    }

                                                    // 2. read HTTP reply
                                                    char buffer [HTTPS_REPLY_BUFFER_SIZE];
                                                    int receivedThisTime;

                                                    while (true) { // read blocks of incoming data
                                                        receivedThisTime = tlsConnection.recvBlock (buffer, HTTPS_REPLY_BUFFER_SIZE - 1);
                                                        if (receivedThisTime <= 0) {
                                                            params->httpsReply = "TLS read error";
                                                            xSemaphoreGive (params->done);
                                                            vTaskDelete (NULL);    
                                                        }

                                                        // block arrived
                                                        buffer [receivedThisTime] = 0;
                                                        if (!params->httpsReply.concat (buffer)) {
                                                            params->httpsReply = "Out of memory";
                                                            xSemaphoreGive (params->done);
                                                            vTaskDelete (NULL);                                                              
                                                        }

                                                        // check if HTTP reply is complete
                                                        char *p = strstr (params->httpsReply.c_str (), "\nContent-Length:");
                                                        if (p) {
                                                            p += 16;
                                                            unsigned int contentLength;
                                                            if (sscanf (p, "%u", &contentLength) == 1) {
                                                                p = strstr (p, "\r\n\r\n"); // the content comes afterwards
                                                                if (p && contentLength == strlen (p + 4)) {
                                                                    xSemaphoreGive (params->done);
                                                                    vTaskDelete (NULL); // success
                                                                }
                                                            }
                                                        }
                                                        // else continue reading
                                                    } // while       
                                                    
                                                    // never executes
                                                    xSemaphoreGive (params->done);
                                                    vTaskDelete (NULL);
                                                  }
                                , "httpsRequest", 18 * 1024, &params, (tskIDLE_PRIORITY + 1), NULL)) {
            xSemaphoreTake (params.done, portMAX_DELAY);
        } else {
            params.httpsReply = "Out of memory";
        }

        tlsSystem.Cleanup (); // wolfSSL_Cleanup ();

        return params.httpsReply;
    }

#endif