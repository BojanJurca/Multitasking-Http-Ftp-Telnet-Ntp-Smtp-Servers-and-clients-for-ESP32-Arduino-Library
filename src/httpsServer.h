/*

    httpsServer.h 
  
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
#ifndef __HTTPS_SERVER_H__
    #define __HTTPS_SERVER_H__


    #include "httpServer.h"
    #include "tlsConnection.h" // uses WolfSSL library
    


    // ----- TUNING PARAMETERS -----

    #define HTTPS_CONNECTION_STACK_SIZE (18 * 1024)
    #define HTTPS_CONNECTION_TIME_OUT 3


    class httpsServer_t : public httpServer_t {

        public: 


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

                    bool tlsServerHandshake () {
                        // tlsServerHandshake calls wolfSSL_accept which is logically part of accepting ssl connection
                        // logically we would do this in tlsConnection constructor which runs in listener's task 
                        // with very limited stack memory so it was moved here

                        // just pass the call to transport
                        tlsConnection_t *transport = static_cast<tlsConnection_t*>(__transport__);

                        bool b = transport->tlsServerHandshake ();
                        if (b)
                            __cipherName__ = transport->cipherName ();
                        return b;
                    }

                    const char *cipherName () override { return __cipherName__; }

                private:

                    const char *__cipherName__ = "None";
            };


            httpsServer_t (unsigned char *server_key_der, unsigned int server_cert_der_len, unsigned char *server_cert_der, unsigned int server_key_der_len,
                           String (*httpRequestHandlerCallback) (const char *httpRequest, httpConnection_t *hcn) = NULL,
                           void (*wsRequestHandlerCallback) (const char *httpRequest, webSocket_t *webSck) = NULL,
                           int serverPort = 443,
                           bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                           bool runListenerInItsOwnTask = true) : httpServer_t (httpRequestHandlerCallback,
                                                                                wsRequestHandlerCallback,
                                                                                serverPort,
                                                                                firewallCallback,
                                                                                runListenerInItsOwnTask) {
                __setup_SSL__ (server_cert_der, server_cert_der_len, server_key_der, server_key_der_len);                                                                              
            }

            ~httpsServer_t ();


            #ifdef __THREAD_SAFE_FS__
                // this part will not compile with .cpp

        private:

                // constructor with a file system
                void __constructor_with_file_system__ (threadSafeFS::FS& fileSystem,
                                                       String (*httpRequestHandlerCallback) (const char *httpRequest, httpConnection_t *hcn),
                                                       void (*wsRequestHandlerCallback) (const char *httpRequest, webSocket_t *webSck),
                                                       int serverPort,
                                                       bool (*firewallCallback) (char *clientIP, char *serverIP),
                                                       bool runListenerInItsOwnTask) {

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
                    unsigned char *server_cert_der;
                    unsigned int server_cert_der_len;
                    threadSafeFS::File f = fileSystem.open ("/etc/ssl/certs/server-cert.der", "r");
                    if (f) {
                        server_cert_der = (unsigned char *) malloc (f.size ());
						// server_cert_der = (unsigned char *) heap_caps_malloc (f.size (), MALLOC_CAP_32BIT);
                        if (server_cert_der) {
                            if (f.read (server_cert_der, f.size ()) == f.size ()) {
                                server_cert_der_len = f.size ();
                            } else {
                                free (server_cert_der);
                                cout << ( dmesgQueue << "[httpsServer] " "can't read /etc/ssl/certs/server-cert.der" );
                                return;
                            }
                        } else {
                            dmesgQueue << "[httpsServer] " "out of memory, couldn't allocate " << f.size () << " bytes";
                            cout << "[httpsServer] " "out of memory, couldn't allocate " << f.size () << " bytes" " [" << __FILE__ << ", " << __LINE__ << ", " << __func__ << "]\r\n";
                            return;
                        }
                        f.close ();
                    } else {
                        cout << ( dmesgQueue << "[httpsServer] " "can't read /etc/ssl/certs/server-cert.der" );
                        return;
                    }

                    // read server-key.der
                    unsigned char *server_key_der;
                    unsigned int server_key_der_len;
                    f = fileSystem.open ("/etc/ssl/certs/server-key.der", "r");
                    if (f) {
                        server_key_der = (unsigned char *) malloc (f.size ());
                        if (server_key_der) {
                            if (f.read (server_key_der, f.size ()) == f.size ()) {
                                server_key_der_len = f.size ();
                            } else {
                                free (server_cert_der);
                                free (server_key_der);
                                cout << ( dmesgQueue << "[httpsServer] " "can't read /etc/ssl/certs/server-key.der" );
                                return;
                            }
                        } else {
                            free (server_cert_der);
                            dmesgQueue << "[httpsServer] " "out of memory, couldn't allocate " << f.size () << " bytes";
                            cout << "[httpsServer] " "out of memory, couldn't allocate " << f.size () << " bytes" " [" << __FILE__ << ", " << __LINE__ << ", " << __func__ << "]\r\n";
                            return;
                        }
                        f.close ();
                    } else {
                        free (server_cert_der);
                        cout << ( dmesgQueue << "[httpsServer] " "can't read /etc/ssl/certs/server-key.der" );
                        return;
                    }

                    if (!__setup_SSL__ (server_cert_der, server_cert_der_len, server_key_der, server_key_der_len)) {
                        cout << ( dmesgQueue << "[httpsServer] " "can't setup SSL" );
                    }

                    free (server_key_der);
                    free (server_cert_der);

                    return;
                }

        public:

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

                    __constructor_with_file_system__ (fileSystem, httpRequestHandlerCallback, wsRequestHandlerCallback, serverPort, firewallCallback, runListenerInItsOwnTask);
                }

                #if TSFS_FS_COUNT == 1 // there is only one file system wrapped ...
                    httpsServer_t (String (*httpRequestHandlerCallback) (const char *httpRequest, httpConnection_t *hcn) = NULL,
                                   void (*wsRequestHandlerCallback) (const char *httpRequest, webSocket_t *webSck) = NULL,
                                   int serverPort = 443,
                                   bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                   bool runListenerInItsOwnTask = true) : httpServer_t (tsfs, // ... use this one
                                                                                        httpRequestHandlerCallback,
                                                                                        wsRequestHandlerCallback,
                                                                                        serverPort,
                                                                                        firewallCallback,
                                                                                        runListenerInItsOwnTask) {

                        __constructor_with_file_system__ (tsfs, httpRequestHandlerCallback, wsRequestHandlerCallback, serverPort, firewallCallback, runListenerInItsOwnTask);
                    }
                #endif

            #endif


            bool __setup_SSL__ (unsigned char *server_cert_der, unsigned int server_cert_der_len, unsigned char *server_key_der, unsigned int server_key_der_len);

            tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) override;

            // accept any connection, the client will get notified in __createConnectionInstance__
            inline tcpConnection_t *accept () __attribute__((always_inline)) { 
                if (heap_caps_get_largest_free_block (MALLOC_CAP_INTERNAL) < HTTPS_CONNECTION_STACK_SIZE) { 
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

            WOLFSSL_CTX* __ctx__ = NULL;
    };


    httpsServer_t::~httpsServer_t () {
        if (__ctx__) {
            wolfSSL_CTX_free (__ctx__);
            __ctx__ = NULL;
        }
        tlsSystem.Cleanup (); // wolfSSL_Cleanup ();
    }

    bool httpsServer_t::__setup_SSL__ (unsigned char *server_cert_der, unsigned int server_cert_der_len, unsigned char *server_key_der, unsigned int server_key_der_len) {
        #define CTX_CA_CERT_TYPE WOLFSSL_FILETYPE_ASN1 // for binary .der certificate format
        #define CTX_SERVER_KEY_TYPE WOLFSSL_FILETYPE_ASN1 // for binary .der format

        randomSeed (esp_random ());

        // Initialize wolfSSL before assigning ctx
        if (tlsSystem.Init () != WOLFSSL_SUCCESS) { // if (wolfSSL_Init () != WOLFSSL_SUCCESS) {
            dmesgQueue << "[httpsServer] " "wolfSSL_Init failed";
            Serial.printf ("[httpsServer] " "wolfSSL_Init failed" " free=%u largest=%u min=%u %s, %i, %s\n", heap_caps_get_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT), heap_caps_get_minimum_free_size (MALLOC_CAP_DEFAULT), __FILE__, __LINE__, __func__);
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
            tlsSystem.Cleanup (); // wolfSSL_Cleanup ();
            return false;
        }

        __ctx__ = wolfSSL_CTX_new (method);
        if (__ctx__ == NULL) {
            dmesgQueue << "[httpsServer] " "wolfSSL_CTX_new failed";
            Serial.printf ("[httpsServer] " "wolfSSL_CTX_new failed" " free=%u largest=%u min=%u %s, %i, %s\n", heap_caps_get_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT), heap_caps_get_minimum_free_size (MALLOC_CAP_DEFAULT), __FILE__, __LINE__, __func__);
            tlsSystem.Cleanup (); // wolfSSL_Cleanup ();
            return false;
        }                    

        // Serial.println("Initializing certificates...");
        if (wolfSSL_CTX_use_certificate_buffer (__ctx__, server_cert_der, server_cert_der_len, CTX_CA_CERT_TYPE) != WOLFSSL_SUCCESS) {
			int err = wolfSSL_get_error (NULL, 0); 
			cout << ( dmesgQueue << "[httpsServer] " "wolfSSL_CTX_use_certificate_buffer failed: " << err ); // wolfSSL_ERR_reason_error_string (err) );
            tlsSystem.Cleanup (); // wolfSSL_Cleanup ();
            return false;
        }

        // Setup private server key
        if (wolfSSL_CTX_use_PrivateKey_buffer (__ctx__, server_key_der, server_key_der_len, CTX_SERVER_KEY_TYPE) != WOLFSSL_SUCCESS) {
			int err = wolfSSL_get_error (NULL, 0); 
			cout << ( dmesgQueue << "[httpsServer] " "wolfSSL_CTX_use_PrivateKey_buffer failed: " << err ); // wolfSSL_ERR_reason_error_string (err) );
            tlsSystem.Cleanup (); // wolfSSL_Cleanup ();
            return false;
        }

        // Use built-in validation, No verification callback function:
        wolfSSL_CTX_set_verify (__ctx__, SSL_VERIFY_NONE, 0);

        return true;
    }


    tcpConnection_t *httpsServer_t::__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) {
        tlsConnection_t *tlsConnection = new (std::nothrow) tlsConnection_t (connectionSocket, clientIP, serverIP, __ctx__);
        if (!tlsConnection) {
            dmesgQueue << "[httpsServer] " "can't create connection instance, out of memory";
            cout << "[httpsServer] " "can't create connection instance, out of memory" " [" << __FILE__ << ", " << __LINE__ << ", " << __func__ << "]\r\n";
            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            close (connectionSocket); // normally tcpConnection would do this but if it is not created we have to do it here since the connection was not created
            xSemaphoreGive (getLwIpMutex ());
            return NULL;
        }

        if (tlsConnection->errNo ()) {
            cout << ( dmesgQueue << "[httpsServer] " "can't create connection instance, " << tlsConnection->errText () );
            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            close (connectionSocket); // normally tcpConnection would do this but if it is not created we have to do it here since the connection was not created
            xSemaphoreGive (getLwIpMutex ());
            return NULL;
        }

        httpsConnection_t *httpsConnection = new (std::nothrow) httpsConnection_t (tlsConnection, __fileSystem__, __replyWithFileContentPtr__, __httpRequestHandlerCallback__, __wsRequestHandlerCallback__);
        if (!httpsConnection) {
            dmesgQueue << "[httpsServer] " "can't create connection instance, out of memory";
            cout << "[httpsServer] " "can't create connection instance, out of memory" " [" << __FILE__ << ", " << __LINE__ << ", " << __func__ << "]\r\n";
            delete (tlsConnection);
            return NULL;
        }

        tlsConnection->setIdleTimeout (HTTPS_CONNECTION_TIME_OUT);    

        if (pdPASS != xTaskCreate ([] (void *thisInstance) {
                                                                httpsConnection_t* ths = static_cast<httpsConnection_t*>(thisInstance); // get "this" pointer

                                                                // tlsServerHandshake calls wolfSSL_accept which is logically part of accepting ssl connection
                                                                // logically we would do this in tlsConnection constructor which runs in listener's task 
                                                                // with very limited stack memory so it was moved here
                                                                if (ths->tlsServerHandshake ()) {

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

            dmesgQueue << "[httpsServer] " "can't create connection task, out of memory";
            cout << "[httpsServer] " "can't create connection task, out of memory" " [" << __FILE__ << ", " << __LINE__ << ", " << __func__ << "]\r\n";

            delete (httpsConnection); // (httpsConnection will delete tcpConnection itself) normally tcpConnection would do this but if it is not running we have to do it here
            return NULL;
        }

        return NULL; // success, but don't return connection, since it may already been closed and destructed by now
    }

#endif