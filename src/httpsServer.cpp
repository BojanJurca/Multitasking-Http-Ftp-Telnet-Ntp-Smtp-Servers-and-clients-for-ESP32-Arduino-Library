/*

    httpsServer.cpp
  
    This file is part of Multitasking Esp32 HTTP FTP http servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-http-servers-for-Arduino
  
    May 22, 2026, Bojan Jurca


    Extension of httpServer_t with WolfSSL library

*/


#include "httpsServer.h"

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


// vse inicializaciej daj sem - kličeš iz več konstruktorjev
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
