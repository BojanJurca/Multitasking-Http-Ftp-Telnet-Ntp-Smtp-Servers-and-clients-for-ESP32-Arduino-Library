/*

    httpsServer.h
  
    This file is part of Multitasking Esp32 HTTP FTP http servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-http-servers-for-Arduino
  
    May 22, 2026, Bojan Jurca


    Extension of httpServer_t with WolfSSL library

*/


#pragma once
#ifndef __HTTPS_SERVER__
    #define __HTTPS_SERVER__

    #include <httpServer.h>


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


    class tlsConnection_t : public tcpConnection_t {

        friend class httpsConnection_t;

        public:

            tlsConnection_t (int socket, const char* clientIP, const char* serverIP, WOLFSSL_CTX* ctx);
            
            bool tlsHandshake ();

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

            WOLFSSL *__ssl__ = NULL;            
            const char *__cipherName__ = "";
    };

    class httpsConnection_t : public httpServer_t::webSocket_t {

        public:

            httpsConnection_t (tlsConnection_t* transport,
                            void *FileSystem,
                            bool (webSocket_t::*replyWithFileContentPtr) (),
                            String (*httpRequestHandlerCallback) (const char *httpRequest, httpServer_t::httpConnection_t *hcn),
                            void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck)) 
                            : 
                            httpServer_t::webSocket_t (transport,
                                                        FileSystem,
                                                        replyWithFileContentPtr,
                                                        httpRequestHandlerCallback,
                                                        wsRequestHandlerCallback) {
            }

            bool tlsHandshake () {
                // tlsHandshake calls wolfSSL_accept which is logically part of accepting ssl connection
                // logically we would do this in tlsConnection constructor which runs in listener's task 
                // with very limited stack memory so it was moved here

                // samo poda naprej
                tlsConnection_t *transport = static_cast<tlsConnection_t*>(__transport__);

                bool b = transport->tlsHandshake ();
                if (b)
                    __cipherName__ = transport->cipherName ();
                return b;
            }

            const char *cipherName () override { return __cipherName__; }

        private:

            const char *__cipherName__ = "";
    };


    // ----- TUNING PARAMETERS -----


    #define HTTPS_CONNECTION_STACK_SIZE (17 * 1024)
    #define HTTPS_CONNECTION_TIME_OUT 3


    class httpsServer_t : public httpServer_t {

        public: 

            httpsServer_t (unsigned char *__server_key_der__, unsigned int __server_cert_der_len__, unsigned char *__server_cert_der__, unsigned int __server_key_der_len__,
                        String (*httpRequestHandlerCallback) (const char *httpRequest, httpConnection_t *hcn) = NULL,
                        void (*wsRequestHandlerCallback) (const char *httpRequest, webSocket_t *webSck) = NULL,
                        int serverPort = 443,
                        bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                        bool runListenerInItsOwnTask = true) : httpServer_t (httpRequestHandlerCallback,
                                                                                wsRequestHandlerCallback,
                                                                                serverPort,
                                                                                firewallCallback,
                                                                                runListenerInItsOwnTask) {
                __setup_SSL__ (__server_key_der__, __server_cert_der_len__, __server_cert_der__, __server_key_der_len__);                                                                              
            }

            ~httpsServer_t ();

            #ifdef __THREAD_SAFE_FS__
                // this part will not compile with .cpp

                // constructor with a file system
                httpsServer_t (threadSafeFS::FS& fileSystem,
                            String (*httpRequestHandlerCallback) (const char *httpRequest, httpConnection_t *hcn) = NULL,
                            void (*wsRequestHandlerCallback) (const char *httpRequest, webSocket_t *webSck) = NULL,
                            int serverPort = 443,
                            bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                            bool runListenerInItsOwnTask = true) : httpServer_t (fileSystem,
                                                                                    httpRequestHandlerCallback,
                                                                                    wsRequestHandlerCallback,
                                                                                    serverPort,
                                                                                    firewallCallback,
                                                                                    runListenerInItsOwnTask) {

                    __freeBuffersWhenDestructed__ = true;

                    // create directory structure and test sever-key.der and server-cert.der files
                    if (!fileSystem.isDirectory ("/etc/ssl/certs")) {
                        fileSystem.mkdir ("/etc");
                        fileSystem.mkdir ("/etc/ssl");
                        fileSystem.mkdir ("/etc/ssl/certs");
                        if (!fileSystem.isDirectory ("/etc/ssl/certs"))
                            cout << ( dmesgQueue << "[httpsServer] " "can't create /etc/ssl/certs" );
                    }


                    if (!fileSystem.isFile ("/etc/ssl/certs/server-cert.der")) {
                        threadSafeFS::File f = fileSystem.open ("/etc/ssl/certs/server-cert.der", "w");
                        if (f) {
                            const static unsigned char server_cert_der [] = {
                                0x30, 0x82, 0x01, 0xf0, 0x30, 0x82, 0x01, 0x96, 0xa0, 0x03, 0x02, 0x01,
                                0x02, 0x02, 0x14, 0x4d, 0xb4, 0xe1, 0x57, 0x0e, 0x7b, 0x5f, 0xff, 0xcc,
                                0xad, 0xc3, 0x56, 0xde, 0xc3, 0x0c, 0xbb, 0xc3, 0x08, 0x7c, 0x84, 0x30,
                                0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x30,
                                0x5e, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
                                0x53, 0x49, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c,
                                0x08, 0x53, 0x6c, 0x6f, 0x76, 0x65, 0x6e, 0x69, 0x61, 0x31, 0x12, 0x30,
                                0x10, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0c, 0x09, 0x4c, 0x6a, 0x75, 0x62,
                                0x6c, 0x6a, 0x61, 0x6e, 0x61, 0x31, 0x0e, 0x30, 0x0c, 0x06, 0x03, 0x55,
                                0x04, 0x0a, 0x0c, 0x05, 0x45, 0x53, 0x50, 0x33, 0x32, 0x31, 0x18, 0x30,
                                0x16, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x0f, 0x6a, 0x75, 0x72, 0x63,
                                0x61, 0x2e, 0x64, 0x79, 0x6e, 0x2e, 0x74, 0x73, 0x2e, 0x73, 0x69, 0x30,
                                0x1e, 0x17, 0x0d, 0x32, 0x36, 0x30, 0x35, 0x30, 0x35, 0x30, 0x37, 0x34,
                                0x36, 0x35, 0x31, 0x5a, 0x17, 0x0d, 0x33, 0x36, 0x30, 0x35, 0x30, 0x32,
                                0x30, 0x37, 0x34, 0x36, 0x35, 0x31, 0x5a, 0x30, 0x5e, 0x31, 0x0b, 0x30,
                                0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x53, 0x49, 0x31, 0x11,
                                0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x08, 0x53, 0x6c, 0x6f,
                                0x76, 0x65, 0x6e, 0x69, 0x61, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55,
                                0x04, 0x07, 0x0c, 0x09, 0x4c, 0x6a, 0x75, 0x62, 0x6c, 0x6a, 0x61, 0x6e,
                                0x61, 0x31, 0x0e, 0x30, 0x0c, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x05,
                                0x45, 0x53, 0x50, 0x33, 0x32, 0x31, 0x18, 0x30, 0x16, 0x06, 0x03, 0x55,
                                0x04, 0x03, 0x0c, 0x0f, 0x6a, 0x75, 0x72, 0x63, 0x61, 0x2e, 0x64, 0x79,
                                0x6e, 0x2e, 0x74, 0x73, 0x2e, 0x73, 0x69, 0x30, 0x59, 0x30, 0x13, 0x06,
                                0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86,
                                0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0xcd, 0x9f,
                                0x1e, 0xc1, 0x2e, 0x94, 0x56, 0xd9, 0x8d, 0x7b, 0x78, 0xd8, 0xed, 0xa3,
                                0x14, 0x11, 0x8c, 0xec, 0x30, 0x09, 0x29, 0x19, 0xde, 0xed, 0xaf, 0x70,
                                0x6b, 0x2a, 0x87, 0xc4, 0x60, 0x21, 0xa8, 0xd2, 0xe2, 0x2a, 0x1a, 0xda,
                                0x97, 0xaf, 0x3e, 0xf8, 0x2c, 0x4e, 0x8b, 0x0a, 0x59, 0x2d, 0xe9, 0x50,
                                0x84, 0x2f, 0x58, 0x05, 0x17, 0xe6, 0x43, 0xbe, 0x42, 0x65, 0x11, 0x86,
                                0x99, 0x49, 0xa3, 0x32, 0x30, 0x30, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d,
                                0x0e, 0x04, 0x16, 0x04, 0x14, 0xf8, 0xde, 0x17, 0x6c, 0xea, 0x68, 0x81,
                                0x1c, 0xd4, 0x9e, 0x68, 0xa5, 0x97, 0x3d, 0x98, 0x09, 0x13, 0x5f, 0x64,
                                0x60, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04,
                                0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86,
                                0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x48, 0x00, 0x30, 0x45, 0x02,
                                0x21, 0x00, 0x9d, 0xed, 0x1f, 0x59, 0x09, 0xca, 0x52, 0x7a, 0x17, 0x5c,
                                0xf0, 0x5a, 0x98, 0x9e, 0x6d, 0x2f, 0x17, 0x23, 0xb8, 0x35, 0xac, 0x06,
                                0x7c, 0xda, 0xd0, 0x4e, 0xde, 0x42, 0x3f, 0xcb, 0x05, 0xad, 0x02, 0x20,
                                0x15, 0x8f, 0x37, 0xd0, 0xc4, 0xc6, 0x70, 0x4c, 0x80, 0x4f, 0xa0, 0x93,
                                0xab, 0x20, 0x44, 0x77, 0x85, 0xcb, 0xa4, 0xe2, 0x56, 0x0a, 0xa3, 0xe3,
                                0x88, 0x58, 0xf5, 0x4e, 0x2a, 0x67, 0xf2, 0x2f
                            };
                            f.write (server_cert_der, sizeof (server_cert_der));
                            f.close ();
                        }
                    }

                    if (!fileSystem.isFile ("/etc/ssl/certs/server-key.der")) {
                        threadSafeFS::File f = fileSystem.open ("/etc/ssl/certs/server-key.der", "w");
                        if (f) {
                            const static unsigned char server_key_der [] = {
                                0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0x1c, 0xc0, 0xae, 0x64, 0x4b,
                                0x63, 0xb0, 0x40, 0x24, 0x70, 0x12, 0x13, 0x3f, 0xda, 0x5c, 0x9b, 0x08,
                                0x58, 0xc3, 0xc8, 0x79, 0x77, 0xcc, 0x0e, 0xcc, 0x20, 0xea, 0x04, 0x3d,
                                0x9d, 0x28, 0xd3, 0xa0, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,
                                0x03, 0x01, 0x07, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0xcd, 0x9f, 0x1e,
                                0xc1, 0x2e, 0x94, 0x56, 0xd9, 0x8d, 0x7b, 0x78, 0xd8, 0xed, 0xa3, 0x14,
                                0x11, 0x8c, 0xec, 0x30, 0x09, 0x29, 0x19, 0xde, 0xed, 0xaf, 0x70, 0x6b,
                                0x2a, 0x87, 0xc4, 0x60, 0x21, 0xa8, 0xd2, 0xe2, 0x2a, 0x1a, 0xda, 0x97,
                                0xaf, 0x3e, 0xf8, 0x2c, 0x4e, 0x8b, 0x0a, 0x59, 0x2d, 0xe9, 0x50, 0x84,
                                0x2f, 0x58, 0x05, 0x17, 0xe6, 0x43, 0xbe, 0x42, 0x65, 0x11, 0x86, 0x99,
                                0x49
                            };
                            f.write (server_key_der, sizeof (server_key_der));
                            f.close ();
                        }
                    }

                    // read server-cert.der
                    threadSafeFS::File f = fileSystem.open ("/etc/ssl/certs/server-cert.der", "r");
                    if (f) {
                        __server_cert_der__ = (unsigned char *) malloc (f.size ());
                        if (__server_cert_der__) {
                            if (f.read (__server_cert_der__, f.size ()) == f.size ()) {
                                __server_cert_der_len__ = f.size ();
                            } else {
                                free (__server_cert_der__);
                                cout << ( dmesgQueue << "[httpsServer] " "can't read /etc/ssl/certs/server-cert.der" );
                                return;
                            }
                        } else {
                            cout << ( dmesgQueue << "[httpsServer] " "out of memory" );
                            return;
                        }
                        f.close ();
                    } else {
                        cout << ( dmesgQueue << "[httpsServer] " "can't read /etc/ssl/certs/server-cert.der" );
                        return;
                    }

                    // read server-key.der
                    f = fileSystem.open ("/etc/ssl/certs/server-key.der", "r");
                    if (f) {
                        __server_key_der__ = (unsigned char *) malloc (f.size ());
                        if (__server_key_der__) {
                            if (f.read (__server_key_der__, f.size ()) == f.size ()) {
                                __server_key_der_len__ = f.size ();
                            } else {
                                free (__server_cert_der__);
                                free (__server_key_der__);
                                cout << ( dmesgQueue << "[httpsServer] " "can't read /etc/ssl/certs/server-key.der" );
                                return;
                            }
                        } else {
                            free (__server_cert_der__);
                            cout << ( dmesgQueue << "[httpsServer] " "out of memory" );
                            return;
                        }
                        f.close ();
                    } else {
                        free (__server_cert_der__);
                        cout << ( dmesgQueue << "[httpsServer] " "can't read /etc/ssl/certs/server-key.der" );
                        return;
                    }


                    if (!__setup_SSL__ (__server_key_der__, __server_cert_der_len__, __server_cert_der__, __server_key_der_len__)) {
                        free (__server_key_der__);
                        free (__server_cert_der__);
                        __server_key_der__ = __server_cert_der__ = NULL;
                    }
                }

            #endif



            static int IORecvCallback (WOLFSSL* ssl, char* buf, int sz, void* ctx);
            static int IOSendCallback (WOLFSSL* ssl, char* buf, int sz, void* ctx);

            bool __setup_SSL__ (unsigned char *__server_key_der__, unsigned int __server_cert_der_len__, unsigned char *__server_cert_der__, unsigned int __server_key_der_len__);

            tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) override;

            // accept any connection, the client will get notified in __createConnectionInstance__
            inline tcpConnection_t *accept () __attribute__((always_inline)) { 
                if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) < HTTPS_CONNECTION_STACK_SIZE) { 
                    // There is not a memory block large enough evailable to start new task that would handle the new connection.
                    // If we ::accept () the connection now we would only have to report503 "HTTP/1.0 503 Service unavailable
                    // to the client later. But if we don't call ::accept () now the incoming connection will wait for a while,
                    // and perhaps get ::acceptted () a few moments later 
                    return NULL;
                } else {
                    return tcpServer_t::accept (); 
                }
            }

        private:

            bool __freeBuffersWhenDestructed__ = false;
            unsigned char *__server_key_der__ = NULL;
            unsigned int __server_cert_der_len__ = 0;
            unsigned char *__server_cert_der__ = NULL;
            unsigned int __server_key_der_len__ = 0;

            WOLFSSL_CTX* __ctx__ = NULL;
    };




    #define CTX_CA_CERT_TYPE WOLFSSL_FILETYPE_ASN1 // for binary .der format


    // ----- tlsConnection_t -----

    tlsConnection_t::tlsConnection_t (int socket, const char* clientIP, const char* serverIP, WOLFSSL_CTX* ctx) : tcpConnection_t (socket, clientIP, serverIP) {
        // SSL init
        __ssl__ = wolfSSL_new (ctx);
        if (!__ssl__) {
            cout << ( dmesgQueue << "[tlsConn] " "wolfSSL_new failed" );
            return;
        }

        // bind ssl to socket
        wolfSSL_set_fd (__ssl__, socket);

        // "this" will be passed as ctx to IORecvCallback and IOSendCallback functions
        wolfSSL_SetIOReadCtx (__ssl__, this);
        wolfSSL_SetIOWriteCtx (__ssl__, this);
    }

    bool tlsConnection_t::tlsHandshake () {
        // tlsHandshake calls wolfSSL_accept which is logically part of accepting ssl connection
        // logically we would do this in tlsConnection constructor which runs in listener's task 
        // with very limited stack memory so it was moved here

        int ret = wolfSSL_accept (__ssl__); 
        if (ret != WOLFSSL_SUCCESS) {
            char wc_error_message [81];
            int err = wolfSSL_get_error (__ssl__, ret);
            wolfSSL_ERR_error_string (err, wc_error_message);
            cout << "[tlsConn] " "wolfSSL_accept failed: " << wc_error_message;
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
    }

    int tlsConnection_t::recvBlock (void *buf, size_t len) {
        int received = wolfSSL_read (__ssl__, buf, len);
        if (received <= 0) {
            char wc_error_message [81];
            int err = wolfSSL_get_error (__ssl__, received);
            wolfSSL_ERR_error_string (err, wc_error_message);
            cout << "[tlsConn] " "wolfSSL_read failed: " << wc_error_message;
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
                cout << "[tlsConn] " "wolfSSL_read failed: " << wc_error_message;
                return receivedThisTime;
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
                cout << "[tlsConn] " "wolfSSL_write failed: " << wc_error_message;
                return sentThisTime;
            }
        
            sentTotal += sentThisTime;

            if (sentTotal < len) 
                delay (25);
        } 
        return sentTotal;
    }


    // ----- httpsServer_t -----

    httpsServer_t::~httpsServer_t () {
        if (__ctx__) {
            wolfSSL_CTX_free (__ctx__);
            __ctx__ = NULL;
        }
        wolfSSL_Cleanup ();
        if (__freeBuffersWhenDestructed__) {
        if (__server_key_der__) free (__server_key_der__);
        if (__server_cert_der__) free (__server_cert_der__);
        }
    }

    // Custom IO functions with LwIP semaphore
    int httpsServer_t::IORecvCallback (WOLFSSL* ssl, char* buf, int sz, void* ctx) {
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
    int httpsServer_t::IOSendCallback (WOLFSSL* ssl, char* buf, int sz, void* ctx) {
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


    bool httpsServer_t::__setup_SSL__ (unsigned char *__server_key_der__, unsigned int __server_cert_der_len__, unsigned char *__server_cert_der__, unsigned int __server_key_der_len__) {
        #define CTX_SERVER_KEY_TYPE WOLFSSL_FILETYPE_ASN1 // for binary .der format

        randomSeed (esp_random ());

        // Initialize wolfSSL before assigning ctx
        if (wolfSSL_Init () != WOLFSSL_SUCCESS) {
            cout << ( dmesgQueue << "[httpsServer] " "wolfSSL_Init failed" );
            return false;
        }

        WOLFSSL_METHOD* method;
        /* See companion server example with wolfSSLv23_server_method here.
        * method = wolfSSLv23_client_method());   SSL 3.0 - TLS 1.3.
        * method = wolfTLSv1_2_client_method();   only TLS 1.2
        * method = wolfTLSv1_3_client_method();   only TLS 1.3
        *
        * see Arduino\libraries\wolfssl\src\user_settings.h */
        method = wolfSSLv23_server_method ();
        if (method == NULL) {
            cout << ( dmesgQueue << "[httpsServer] " "wolfSSLv23_server_method failed" );
            wolfSSL_Cleanup ();
            return false;
        }

        __ctx__ = wolfSSL_CTX_new (method);
        if (__ctx__ == NULL) {
            cout << ( dmesgQueue << "[httpsServer] " "wolfSSL_CTX_new failed" );
            wolfSSL_Cleanup ();
            return false;
        }                    

        // Use built-in validation, No verification callback function:
        wolfSSL_CTX_set_verify (__ctx__, SSL_VERIFY_NONE, 0);

        char wc_error_message [81];

        // Serial.println("Initializing certificates...");
        if (wolfSSL_CTX_use_certificate_buffer (__ctx__, __server_cert_der__, __server_cert_der_len__, CTX_CA_CERT_TYPE) != WOLFSSL_SUCCESS) {
            cout << ( dmesgQueue << "[httpsServer] " "wolfSSL_CTX_use_certificate_buffer failed: " << wc_error_message );
            wolfSSL_Cleanup ();
            return false;
        }

        // Setup private server key
        if (wolfSSL_CTX_use_PrivateKey_buffer (__ctx__, __server_key_der__, __server_key_der_len__, CTX_SERVER_KEY_TYPE) != WOLFSSL_SUCCESS) {
            cout << ( dmesgQueue << "[httpsServer] " "wolfSSL_CTX_use_PrivateKey_buffer failed: " << wc_error_message );
            wolfSSL_Cleanup ();
            return false;
        }

        // Initialize wolfSSL using callback functions.
        wolfSSL_SetIORecv (__ctx__, IORecvCallback); 
        wolfSSL_SetIOSend (__ctx__, IOSendCallback);

        // cout << ( dmesgQueue << "[httpsServer] " "wolfSSL set up" );

        return true;
    }


    tcpConnection_t *httpsServer_t::__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) {

        tlsConnection_t *tlsConnection = new (std::nothrow) tlsConnection_t (connectionSocket, clientIP, serverIP, __ctx__);
        if (!tlsConnection) {
            cout << ( dmesgQueue << "[httpsServer] " "can't create connection instance, out of memory" );
            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            close (connectionSocket); // normally tcpConnection would do this but if it is not created we have to do it here since the connection was not created
            xSemaphoreGive (getLwIpMutex ());
            return NULL;
        }

        httpsConnection_t *httpsConnection = new (std::nothrow) httpsConnection_t (tlsConnection, __fileSystem__, __replyWithFileContentPtr__, __httpRequestHandlerCallback__, __wsRequestHandlerCallback__);
        if (!httpsConnection) {
            cout << ( dmesgQueue << "[httpsServer] " "can't create connection instance, out of memory" );
            delete (tlsConnection);
            return NULL;
        }

        tlsConnection->setIdleTimeout (HTTPS_CONNECTION_TIME_OUT);    

        if (pdPASS != xTaskCreate ([] (void *thisInstance) {
                                                                httpsConnection_t* ths = static_cast<httpsConnection_t*>(thisInstance); // get "this" pointer

                                                                // tlsHandshake calls wolfSSL_accept which is logically part of accepting ssl connection
                                                                // logically we would do this in tlsConnection constructor which runs in listener's task 
                                                                // with very limited stack memory so it was moved here
                                                                if (ths->tlsHandshake ()) {

                                                                    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                                                        __runningTcpConnections__ ++;
                                                                    xSemaphoreGive (getLwIpMutex ());

                                                                    ths->__runConnectionTask__ ();

                                                                    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                                                        __runningTcpConnections__ --;
                                                                    xSemaphoreGive (getLwIpMutex ());

                                                                }

                                                                delete ths;
                                                                vTaskDelete (NULL); // it is connection's responsibility to close itself
                                                            }
                                    , "httpsConn", HTTPS_CONNECTION_STACK_SIZE, httpsConnection, (tskIDLE_PRIORITY + 1), NULL)) {

            cout << ( dmesgQueue << "[httpsServer] " "can't create connection task, out of memory" );
            delete (httpsConnection); // (httpsConnection will delete tcpConnection itself) normally tcpConnection would do this but if it is not running we have to do it here
            return NULL;
        }

        return NULL; // success, but don't return connection, since it may already been closed and destructed by now
    }

#endif

