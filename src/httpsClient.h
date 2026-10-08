/*

    httpsClient.h
  
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  

    Sep 9, 2026, Bojan Jurca


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


    // ----- TUNNING PARAMETERS -----

    #ifndef HTTPS_REPLY_TIME_OUT
        #define HTTPS_REPLY_TIME_OUT 10                 // 10 s
    #endif
    #ifndef HTTPS_REPLY_BUFFER_SIZE
        #define HTTPS_REPLY_BUFFER_SIZE 1440
    #endif


    // ----- CODE -----


    // initializes httpsReply, returns error string or "" for success
    static const char *__httpsRequestWorker__ (String& httpsReply, const char *httpsServer, int httpsPort = 443, const char *httpsAddress = "/", const char *httpsMethod = "GET", unsigned long timeOut = HTTPS_REPLY_TIME_OUT) {

        tlsConnection_t tlsConnection (httpsServer, httpsPort, timeOut);
        if (*tlsConnection.errText ()) {
            return tlsConnection.errText ();
        }

        // 1. send HTTP request
        Cstring<300> httpsRequest;
        httpsRequest += httpsMethod;
        httpsRequest += " ";
        httpsRequest += httpsAddress;
        httpsRequest += " HTTP/1.0\r\nHost: ";
        httpsRequest += httpsServer;
        httpsRequest += "\r\n\r\n"; // 1.0 HTTP does not know keep-alive directive - we want the server to close the connection immediatelly after sending the reply
        if (httpsRequest.errorFlags ()) {
            return "HTTP request too long";
        }

        int sent = tlsConnection.sendString (httpsRequest);
        if (sent <= 0) {
            return "TLS write error";
        }

        // 2. read HTTP reply
        char buffer [HTTPS_REPLY_BUFFER_SIZE];
        int receivedThisTime;

        while (true) { // read blocks of incoming data
            receivedThisTime = tlsConnection.recvBlock (buffer, HTTPS_REPLY_BUFFER_SIZE - 1);
            if (receivedThisTime <= 0) {
                return "TLS read error";
            }

            // block arrived
            buffer [receivedThisTime] = 0;

            if (!httpsReply.concat (buffer)) {
                return "Out of memory, cannot cache the server reply";
            }

            // check if HTTP reply is complete
            char *p = strstr (httpsReply.c_str (), "\nContent-Length:");
            if (p) {
                p += 16;
                unsigned int contentLength;

                if (sscanf (p, "%u", &contentLength) == 1) {
                    p = strstr (p, "\r\n\r\n"); // the content comes afterwards
                    if (p && contentLength == strlen (p + 4)) {
                        return "";
                    }
                }
            }
            // else continue reading
        } // while       

        // never executes
        return "";
    }

    // initializes httpsReply, returns error string or "" for success
    // verifies server certificate against trusted /etc/ssl/ca-trust/*.crt CA certificates
    #ifdef __THREAD_SAFE_FS__
        const char *__httpsRequestWorker__ (threadSafeFS::FS& fileSystem, String& httpsReply, const char *httpsServer, int httpsPort = 443, const char *httpsAddress = "/", const char *httpsMethod = "GET", unsigned long timeOut = HTTPS_REPLY_TIME_OUT, bool verifyServerCertificate = true) {
            // create directory structure and readme.txt file
            if (verifyServerCertificate) {
                if (!fileSystem.isFile ("/etc/ssl/ca-trust/readme.txt")) {
                    fileSystem.mkdir ("/etc");
                    fileSystem.mkdir ("/etc/ssl");
                    fileSystem.mkdir ("/etc/ssl/ca-trust");

                    threadSafeFS::File f = fileSystem.open ("/etc/ssl/ca-trust/readme.txt", "w");
                    if (f) {
                        f.print ("Place trusted CA root certificates (*.crt, DER/ASN.1 format) in this directory.\r\n"
                                "These certificates will be used by HTTPS client to verify remote HTTPS servers\r\n"
                                "or HTTPS server to verify remote HTTPS clients.");
                        f.close ();
                        cout << "Place trusted CA root certificates (*.crt, DER/ASN.1 format) in /etc/ssl/ca-trust directory.\r\nThese certificates will be used by HTTPS cleints to verify remote HTTPS servers\r\n";
                    } else {
                        cout << ( dmesgQueue << "[httpsServer] " "can't create /etc/ssl/ca-trust/readme.txt" );
                    }
                }
            }


            tlsConnection_t tlsConnection (fileSystem, httpsServer, httpsPort, timeOut, verifyServerCertificate);
            if (*tlsConnection.errText ()) {
                return tlsConnection.errText ();
            }

            // 1. send HTTP request
            Cstring<300> httpsRequest;
            httpsRequest += httpsMethod;
            httpsRequest += " ";
            httpsRequest += httpsAddress;
            httpsRequest += " HTTP/1.0\r\nHost: ";
            httpsRequest += httpsServer;
            httpsRequest += "\r\n\r\n"; // 1.0 HTTP does not know keep-alive directive - we want the server to close the connection immediatelly after sending the reply
            if (httpsRequest.errorFlags ()) {
                return "HTTP request too long";
            }

            int sent = tlsConnection.sendString (httpsRequest);
            if (sent <= 0) {
                return "TLS write error";
            }

            // 2. read HTTP reply
            char buffer [HTTPS_REPLY_BUFFER_SIZE];
            int receivedThisTime;

            while (true) { // read blocks of incoming data
                receivedThisTime = tlsConnection.recvBlock (buffer, HTTPS_REPLY_BUFFER_SIZE - 1);
                if (receivedThisTime <= 0) {
                    return "TLS read error";
                }

                // block arrived
                buffer [receivedThisTime] = 0;

                if (!httpsReply.concat (buffer)) {
                    return "Out of memory, cannot cache the server reply";
                }

                // check if HTTP reply is complete
                char *p = strstr (httpsReply.c_str (), "\nContent-Length:");
                if (p) {
                    p += 16;
                    unsigned int contentLength;

                    if (sscanf (p, "%u", &contentLength) == 1) {
                        p = strstr (p, "\r\n\r\n"); // the content comes afterwards
                        if (p && contentLength == strlen (p + 4)) {
                            return "";
                        }
                    }
                }
                // else continue reading
            } // while       

            // never executes
            return "";
        }
    #endif


    class httpsClient_t {
        public:

            // initializes httpsReply, returns error string or "" for success
            const char *httpsRequest (String& httpsReply, const char *httpsServer, int httpsPort = 443, const char *httpsAddress = "/", const char *httpsMethod = "GET", unsigned long timeOut = HTTPS_REPLY_TIME_OUT) {
                if (!WiFi.isConnected () || WiFi.localIP () == IPAddress (0, 0, 0, 0))
                    return "not connected to WiFi";

                // WolfSSL need more stack memory that Arduino normaly provides so run
                // the rest of the code in a separate task and wait for it to finish
                httpsReply = "";
                struct params_t {
                    String& httpsReply;
                    const char *httpsServer;
                    int httpsPort;
                    const char *httpsAddress;
                    const char *httpsMethod;
                    unsigned long timeOut;
                    SemaphoreHandle_t done;
                    const char *retVal;
                } params = { httpsReply, httpsServer, httpsPort, httpsAddress, httpsMethod, timeOut, xSemaphoreCreateBinary (), "" };
                if (pdPASS == xTaskCreate ([] (void *ptr) {
                                                            params_t *params = static_cast<params_t*>(ptr);

                                                            // --- WolfSSL init ---
                                                            tlsSystem.Init (); // wolfSSL_Init ();

                                                            params->retVal = __httpsRequestWorker__ (params->httpsReply, params->httpsServer, params->httpsPort, params->httpsAddress, params->httpsMethod, params->timeOut);

                                                            static UBaseType_t lastHighWaterMark = 19 * 1024;
                                                            UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
                                                            if (lastHighWaterMark > highWaterMark) {
                                                                cout << ( dmesgQueue << "[httpsClient] " << "new stack high water mark: " << highWaterMark << " bytes not used (no server certificate verification)" );
                                                                lastHighWaterMark = highWaterMark;
                                                            }

                                                            tlsSystem.Cleanup (); // wolfSSL_Cleanup ();

                                                            xSemaphoreGive (params->done);
                                                            vTaskDelete (NULL); // success
                                                        }
                                        , "httpsRequest", 19 * 1024, &params, (tskIDLE_PRIORITY + 1), NULL)) {
                    xSemaphoreTake (params.done, portMAX_DELAY);
                } else {
                    params.retVal = "Out of memory, cannot run httpsClient"; 
                }

                return params.retVal;
            }    

            // initializes httpsReply, returns error string or "" for success
            // verifies server certificate against trusted /etc/ssl/ca-trust/*.crt CA certificates
            #ifdef __THREAD_SAFE_FS__
                const char *httpsRequest (threadSafeFS::FS& fileSystem, String& httpsReply, const char *httpsServer, int httpsPort = 443, const char *httpsAddress = "/", const char *httpsMethod = "GET", unsigned long timeOut = HTTPS_REPLY_TIME_OUT) {
                    if (!WiFi.isConnected () || WiFi.localIP () == IPAddress (0, 0, 0, 0))
                        return "not connected to WiFi";

                    // WolfSSL need more stack memory that Arduino normaly provides so run
                    // the rest of the code in a separate task and wait for it to finish
                    httpsReply = "";
                    struct params_t {
                        threadSafeFS::FS& fileSystem;
                        String& httpsReply;
                        const char *httpsServer;
                        int httpsPort;
                        const char *httpsAddress;
                        const char *httpsMethod;
                        unsigned long timeOut;
                        SemaphoreHandle_t done;
                        const char *retVal;
                    } params = { fileSystem, httpsReply, httpsServer, httpsPort, httpsAddress, httpsMethod, timeOut, xSemaphoreCreateBinary (), "" };
                    if (pdPASS == xTaskCreate ([] (void *ptr) {
                                                                params_t *params = static_cast<params_t*>(ptr);

                                                                // --- WolfSSL init ---
                                                                tlsSystem.Init (); // wolfSSL_Init ();

                                                                params->retVal = __httpsRequestWorker__ (params->fileSystem, params->httpsReply, params->httpsServer, params->httpsPort, params->httpsAddress, params->httpsMethod, params->timeOut);

                                                                static UBaseType_t lastHighWaterMark = 19 * 1024 + 512;
                                                                UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
                                                                if (lastHighWaterMark > highWaterMark) {
                                                                    cout << ( dmesgQueue << "[httpsClient] " << "new stack high water mark: " << highWaterMark << " bytes not used (with server certificate verification)" );
                                                                    lastHighWaterMark = highWaterMark;
                                                                }

                                                                tlsSystem.Cleanup (); // wolfSSL_Cleanup ();

                                                                xSemaphoreGive (params->done);
                                                                vTaskDelete (NULL); // success
                                                            }
                                            , "httpsRequest", 19 * 1024 + 512, &params, (tskIDLE_PRIORITY + 1), NULL)) {
                        xSemaphoreTake (params.done, portMAX_DELAY);
                    } else {
                        params.retVal = "Out of memory, cannot run httpsClient";
                    }

                    return params.retVal;
                }
            #endif
    };


    [[deprecated("Use const static char *httpsClient_t ().httpsRequest (String& httpsReply, const char *httpsServer, int httpsPort, const char *httpsAddress, const char *httpsMethod, unsigned long timeOut)")]]
    inline String httpsRequest (const char *httpsServer, int httpsPort = 443, const char *httpsAddress = "/", const char *httpsMethod = "GET", unsigned long timeOut = HTTPS_REPLY_TIME_OUT) {
        String httpsReply;
        const char *retVal = httpsClient_t ().httpsRequest (httpsReply, httpsServer, httpsPort, httpsAddress, httpsMethod);
        if (*retVal) // error
                return (retVal);
        return httpsReply;
    }

#endif