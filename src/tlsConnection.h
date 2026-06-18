/*

    tlsConnection.h
  
    This file is part of ESP32 Multitasking Network Suite: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  

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
#ifndef __TLS_CONNECTION_H__
    #define __TLS_CONNECTION_H__


    #include <WiFi.h>
    #include "tcpConnection.h"
    #include <ostream.hpp>
    #include <Cstring.hpp> 
    #include <dmesg.hpp>


    /* wolfSSL user_settings.h must be included from settings.h
    * Make all configurations changes in user_settings.h
    * Do not edit wolfSSL `settings.h` or `config.h` files.
    * Do not explicitly include user_settings.h in any source code.
    * Each Arduino sketch that uses wolfSSL must have: #include "wolfssl.h"
    * C/C++ source files can use: #include <wolfssl/wolfcrypt/settings.h>
    * The wolfSSL "settings.h" must be included in each source file using wolfSSL.
    * The wolfSSL "settings.h" must appear before any other wolfSSL include.
    */
    #include <wolfssl.h>
    /* Important: make sure settings.h appears before any other wolfSSL headers */
    #include <wolfssl/wolfcrypt/settings.h>
    /* Reminder: settings.h includes user_settings.h
    * For ALL project wolfSSL settings, see:
    * [your path]/Arduino\libraries\wolfSSL\src\user_settings.h   */
    #include <wolfssl/ssl.h>
    #include <wolfssl/wolfcrypt/error-crypt.h>


    class tlsSystem_t {
        private:
            static volatile int initialized;
            static SemaphoreHandle_t mutex;
        public:
            int Init () {
                xSemaphoreTake (mutex, portMAX_DELAY);
                    int ret = WOLFSSL_SUCCESS;
                    if (!initialized)
                        ret = wolfSSL_Init ();
                    if (ret == WOLFSSL_SUCCESS)
                    initialized ++;
                xSemaphoreGive (mutex);
                return ret;
            }
            void Cleanup () {
                xSemaphoreTake (mutex, portMAX_DELAY);
                    initialized --;
                    if (!initialized)
                        wolfSSL_Cleanup ();
                xSemaphoreGive (mutex);
            }
    };
    int volatile tlsSystem_t::initialized = 0;
    SemaphoreHandle_t tlsSystem_t::mutex = xSemaphoreCreateMutex ();
    // global singleton
    tlsSystem_t tlsSystem;


    class tlsConnection_t : public tcpConnection_t {

        friend class httpsConnection_t;

        public:

            // server constructor
            tlsConnection_t (int socket, const char* clientIP, const char* serverIP, WOLFSSL_CTX* ctx);

            // client constructor
            tlsConnection_t (const char *serverName, int serverPort, time_t idleTimeout);
            
            // used when tlsConnection is used by TLS server  
            bool tlsServerHandshake ();

            // used when tlsConnection is used by TLS client  
            bool tlsClientHandshake ();

            ~tlsConnection_t ();

            // bool() operator to test if tlsConnection is ready
            operator bool () const { return __ssl__ != NULL && tcpConnection_t::operator bool (); }

            const char *cipherName () { return __cipherName__; }

            // Override tcpConnection communication functions

            int recvBlock (void *buf, size_t len) override;

            // reads and fills the buffer until endingString is read (also finishes the string with ending 0)
            // returns the number of characters read up to len - 1 if OK
            //                                             len if the buffer is too small
            //                                             0 if the peer closed the connection
            //                                             -1 if error occured
            int recvString (char *buf, size_t len, const char *endingString) override;

            // sends the whole block of charcters
            // returns len in case of OK
            //           0 if the peer closed the connection
            //          -1 in case of error
            int sendBlock (void *buf, size_t len) override;

        private:

            WOLFSSL_CTX *__ctx__ = NULL;
            WOLFSSL *__ssl__ = NULL;            
            const char *__cipherName__ = "";

            static int IORecvCallback (WOLFSSL* ssl, char* buf, int sz, void* ctx);
            static int IOSendCallback (WOLFSSL* ssl, char* buf, int sz, void* ctx);
    };


    // server constructor
    tlsConnection_t::tlsConnection_t (int socket, const char* clientIP, const char* serverIP, WOLFSSL_CTX* ctx) : tcpConnection_t (socket, clientIP, serverIP) {
        // register callbacks
        wolfSSL_SetIORecv (ctx, IORecvCallback);
        wolfSSL_SetIOSend (ctx, IOSendCallback);

        // SSL init
        __ssl__ = wolfSSL_new (ctx);
        if (!__ssl__) {
            __errText__ = "wolfSSL_new failed"; 
            cout << ( dmesgQueue << "[tlsConn] " "wolfSSL_new failed" );
            return;
        }

        // bind ssl to socket
        wolfSSL_set_fd (__ssl__, socket);

        // "this" will be passed as ctx to IORecvCallback and IOSendCallback functions
        wolfSSL_SetIOReadCtx (__ssl__, this);
        wolfSSL_SetIOWriteCtx (__ssl__, this);
    }

    // client constructor
    tlsConnection_t::tlsConnection_t (const char *serverName, int serverPort, time_t idleTimeout) : tcpConnection_t (serverName, serverPort) {
        if (*__errText__)
            return;

        tcpConnection_t::setIdleTimeout (idleTimeout);

        // create context
        __ctx__ = wolfSSL_CTX_new (wolfTLSv1_3_client_method ());
        if (!__ctx__) {
            __errText__ = "wolfSSL_CTX_new failed"; 
            cout << ( dmesgQueue << "[tlsConn] " "wolfSSL_CTX_new failed" );
            return;
        }

        // don't verify server certificate (wolfSSL doesn't support server certificate verification on Arduino)
        wolfSSL_CTX_set_verify (__ctx__, WOLFSSL_VERIFY_NONE, NULL);

        // SNI - uncommnet to use SNI
        // if (wolfSSL_CTX_UseSNI (__ctx__, WOLFSSL_SNI_HOST_NAME, httpsServer, strlen (httpsServer)) != WOLFSSL_SUCCESS) {
        //     return "UseSNI failed";
        // }

        // register callbacks
        wolfSSL_SetIORecv (__ctx__, IORecvCallback);
        wolfSSL_SetIOSend (__ctx__, IOSendCallback);

        // SSL init
        __ssl__ = wolfSSL_new (__ctx__);
        if (!__ssl__) {
            __errText__ = "wolfSSL_new failed";
            cout << ( dmesgQueue << "[tlsConn] " "wolfSSL_new failed" );
            return;
        }

        // bind ssl to socket
        wolfSSL_set_fd (__ssl__, getSocket ());

        // "this" will be passed as ctx to IORecvCallback and IOSendCallback functions
        wolfSSL_SetIOReadCtx (__ssl__, this);
        wolfSSL_SetIOWriteCtx (__ssl__, this);

        tlsClientHandshake ();
        // __errText__ is already set if failed
    }

    bool tlsConnection_t::tlsServerHandshake () {
        // tlsServerHandshake calls wolfSSL_accept which is logically part of accepting ssl connection
        // logically we would do this in tlsConnection constructor which runs in listener's task 
        // with very limited stack memory so it was moved here

        int ret = wolfSSL_accept (__ssl__); 
        if (ret != WOLFSSL_SUCCESS) {
            char wc_error_message [81];
            int err = wolfSSL_get_error (__ssl__, ret);
            wolfSSL_ERR_error_string (err, wc_error_message);
            __errText__ = "wolfSSL_accept failed";
            cout << "[tlsConn] " "wolfSSL_accept failed: " << wc_error_message;
            return false;
        }

        // get cipher only after the handshake is over
        __cipherName__ = wolfSSL_get_cipher (__ssl__);

        return true;
    }

    bool tlsConnection_t::tlsClientHandshake () {
        int ret = wolfSSL_connect (__ssl__);
        if (ret != SSL_SUCCESS) {
            char wc_error_message [81];
            int err = wolfSSL_get_error (__ssl__, ret);
            wolfSSL_ERR_error_string (err, wc_error_message);
            __errText__ = "wolfSSL_connect failed";
            cout << "[tlsConn] " "wolfSSL_connect failed: " << wc_error_message;
            return false;
        }

        // get cipher only after the handshake is over
        __cipherName__ = wolfSSL_get_cipher (__ssl__);

        return true;
    }

    tlsConnection_t::~tlsConnection_t () {
        if (__ssl__) {
            wolfSSL_shutdown (__ssl__);
            wolfSSL_free (__ssl__);                
            __ssl__ = NULL;
        }
        if (__ctx__) {
            wolfSSL_CTX_free (__ctx__);
            __ctx__ = NULL;
        }
    }

    int tlsConnection_t::recvBlock (void *buf, size_t len) {
        int received = wolfSSL_read (__ssl__, buf, len);
        if (received <= 0) {
            char wc_error_message [81];
            int err = wolfSSL_get_error (__ssl__, received);
            wolfSSL_ERR_error_string (err, wc_error_message);
            __errText__ = "wolfSSL_read failed";
            cout << "[tlsConn] " "wolfSSL_read failed: " << wc_error_message;
        } else {
            __errText__ = "";
        }
        return received;
    }

    // reads and fills the buffer until endingString is read (also finishes the string with ending 0)
    // returns the number of characters read up to len - 1 if OK
    //                                             len if the buffer is too small
    //                                             0 if the peer closed the connection
    //                                             -1 if error occured
    int tlsConnection_t::recvString (char *buf, size_t len, const char *endingString) {
        int receivedTotal = 0;
        int receivedThisTime;

        while (receivedTotal != len - 1) { // read blocks of incoming data
            receivedThisTime = wolfSSL_read (__ssl__, buf + receivedTotal, len - receivedTotal - 1);
            if (receivedThisTime <= 0) {
                char wc_error_message [81];
                int err = wolfSSL_get_error (__ssl__, receivedThisTime);
                wolfSSL_ERR_error_string (err, wc_error_message);
                __errText__ = "wolfSSL_read failed";
                cout << "[tlsConn] " "wolfSSL_read failed: " << wc_error_message;
                return receivedThisTime;
            } else {
                __errText__ = "";
            }

            receivedTotal += receivedThisTime;

            // the following code assumes that the other side sends command or reply (according to the protocol) that ends with endingString
            buf [receivedTotal] = 0;
            if (strstr (buf, endingString))
                return receivedTotal;
        }

        // we read the whole buffer up to len-1 but the ending string did not arrive - buffer is too small
        return len; 
    }

    // sends the whole block of charcters
    // returns len in case of OK
    //           0 if the peer closed the connection
    //          -1 in case of error
    int tlsConnection_t::sendBlock (void *buf, size_t len) {
        size_t sentTotal = 0;
        while (sentTotal < len) {
            int sentThisTime = wolfSSL_write (__ssl__, (char *) buf + sentTotal, len - sentTotal); 

            if (sentThisTime <= 0) {
                char wc_error_message [81];
                int err = wolfSSL_get_error (__ssl__, sentThisTime);
                wolfSSL_ERR_error_string (err, wc_error_message);
                __errText__ = "wolfSSL_write failed";
                cout << "[tlsConn] " "wolfSSL_write failed: " << wc_error_message;
                return sentThisTime;
            } else {
                __errText__ = "";
            }
        
            sentTotal += sentThisTime;

            if (sentTotal < len) 
                delay (25);
        } 
        return sentTotal;
    }

    // Custom IO functions with LwIP semaphore
    int tlsConnection_t::IORecvCallback (WOLFSSL* ssl, char* buf, int sz, void* ctx) {
        tcpConnection_t *tcpConnection = static_cast<tcpConnection_t*>(ctx);
        int n = tcpConnection->recv (buf, sz);
        if (n > 0)
            return n;   // OK
        if (n == 0)
            return WOLFSSL_CBIO_ERR_CONN_CLOSE;  // peer closed
        // n < 0 → check errno
        switch (errno) {
            case 11:        // EAGAIN and EWOULDBLOCK
                            return WOLFSSL_CBIO_ERR_WANT_READ;
            case 107:       // ENOTCONN
                            [[fallthrough]];
            case 104:       // ECONNRESET
                            [[fallthrough]];
            case 113:       // ECONNABORTED:
                            [[fallthrough]];
            case 128:       // LWIP internal "socket closed"
                            return WOLFSSL_CBIO_ERR_CONN_CLOSE;
            default:
                            return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }
    int tlsConnection_t::IOSendCallback (WOLFSSL* ssl, char* buf, int sz, void* ctx) {
        tcpConnection_t* tcpConnection = static_cast<tcpConnection_t*>(ctx);
        int n = tcpConnection->send (buf, sz);
        if (n > 0)
            return n;   // OK
        if (n == 0)
            return WOLFSSL_CBIO_ERR_CONN_CLOSE;  // peer closed
        // n < 0 → check errno
        switch (errno) {
            case 11:        // EAGAIN / EWOULDBLOCK
                            return WOLFSSL_CBIO_ERR_WANT_WRITE;
            case 107:       // ENOTCONN
                            [[fallthrough]];
            case 104:       // ECONNRESET
                            [[fallthrough]];
            case 113:       // ECONNABORTED
                            [[fallthrough]];
            case 128:       // LWIP internal "socket closed"
                            return WOLFSSL_CBIO_ERR_CONN_CLOSE;
            default:
                            return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }

#endif