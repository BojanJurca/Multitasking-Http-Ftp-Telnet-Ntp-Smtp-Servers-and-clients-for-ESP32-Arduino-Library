/*

    httpServer.cpp
  
    This file is part of Multitasking Esp32 HTTP FTP http servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-http-servers-for-Arduino
  
    March 12, 2026, Bojan Jurca


    Classes implemented/used in this module:

        httpServer_t
        httpServer_t::httpConnection_t
        httpServer_t::webSocket_t

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


    Nomenclature used here for easier understaning of the code:

     - "connection" normally applies to TCP connection from when it was established to when it is terminated

                  Looking back to HTTP 1.0 protocol one TCP connection was used to transmit oparamene HTTP request from browser to web server
                  and then HTTP response back to the browser. After this TCP connection was terminated. HTTP 1.1 introduced "keep-alive"
                  directive. When the browser requests several pages from server it can do it over the same TCP connection in short time period 
                  (one after another) reducing the need of establishing and terminating TCP connections several times.   

     - "session" applies to user interaction between login and logout

                  Taking account the web technology one web session tipically consists of many TCP connections. Cookies are involved
                  in order to preserve the state of the web session since HTTP requests and replies themselves are stateless. A list of 
                  valid "session tokens" is keept on the web server so web server can check the session validity.

      - "buffer size" is the number of bytes that can be placed in a buffer. 
      
                  In case of C 0-terminated strings the terminating 0 character should be included in "buffer size".


      - "HTTP request":

                  ┌──────────────── GET / HTTP/1.1
                  │                 Host: 10.18.1.200
                  │                 Connection: keep-alive    field value
                  │                 Cache-Control: max-age=0   |
         request  │    field name - Upgrade-Insecure-Requests: 1
                  │                 User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.45 Safari/537.36
                  │                 Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng;q=0.8,application/signed-exchange;v=b3;q=0.9
                  │                 Accept-Encoding: gzip, deflate
                  │                 Accept-Language: sl-SI,sl;q=0.9,en-GB;q=0.8,en;q=0.7
                  └──────────────── Cookie: refreshCounter=1
                                                |          |
                                           cookie name   cookie value

      - "HTTP reply":
                                                    status
                                                       |         cookie name, value, path and expiration
                  ┌─────────┬──────────────── HTTP/1.1 200 OK      |     |    |         |
                  │         │                 Set-Cookie: refreshCounter=2; Path=/; Expires=Thu, 09 Dec 2021 19:07:04 GMT
            reply │  header │    field name - Content-Type: text/html
                  │         │                 Content-Length: 96   |
                  │         └────────────────                     field value
                  └─ content ──────────────── <HTML>Web cookies<br><br>This page has been refreshed 2 times. Click refresh to see more.</HTML>

*/


#include "httpServer.h"






// ----- httpServer_t::webSocket_t implementation -----

// static member initialization
UBaseType_t httpServer_t::webSocket_t::__lastHighWaterMark__ = HTTP_CONNECTION_STACK_SIZE;


// ----- websocket_t implementation -----

httpServer_t::webSocket_t::webSocket_t (
    void *fileSystem,
    bool (webSocket_t::*replyWithFileContentPtr) (),
    int connectionSocket,
    char *clientIP,
    char *serverIP,
    String (*httpRequestHandlerCallback) (const char *httpRequest, httpConnection_t *hcn),
    void (*wsRequestHandlerCallback) (const char *httpRequest, webSocket_t *webSck)
) : tcpConnection_t (connectionSocket, clientIP, serverIP), 
    __fileSystem__ (fileSystem), 
    __replyWithFileContentPtr__ (replyWithFileContentPtr),
    __httpRequestHandlerCallback__ (httpRequestHandlerCallback),
    __wsRequestHandlerCallback__ (wsRequestHandlerCallback)
{
}

httpServer_t::webSocket_t::~webSocket_t () {
    if (__recvFrameBuffer__) { // assuming WebSocket connection has been established
        __sendFrame__ (NULL, 0, CLOSE_FRAME_TYPE); // try sending closing WebSocket frame to the browser
        free (__recvFrameBuffer__);              
    }
}

char *httpServer_t::webSocket_t::getHttpRequest () {
    return __httpRequestAndReplyBuffer__.c_str ();
}

Cstring<300> httpServer_t::webSocket_t::getHttpRequestHeaderField (const char *fieldName) {
    char *p = strstr (__httpRequestAndReplyBuffer__, fieldName);
    if (p && p != __httpRequestAndReplyBuffer__ && *(p - 1) == '\n' && *(p + strlen (fieldName)) == ':') { // p points to fieldName in HTTP request
        p += strlen (fieldName) + 1; 
        while (*p == ' ') 
            p++; // p points to field value in HTTP request
        cstring s; 
        while (*p >= ' ') 
            s += *(p ++);
        return s;
    }
    return "";
}

Cstring<300> httpServer_t::webSocket_t::getHttpRequestCookie (const char *cookieName) {
    char *p = strstr (__httpRequestAndReplyBuffer__, (char *) "\nCookie:"); // find cookie field name in HTTP header          
    if (p) {
        p = strstr (p, cookieName); // find cookie name in HTTP header
        if (p && p != __httpRequestAndReplyBuffer__ && *(p - 1) == ' ' && *(p + strlen (cookieName)) == '=') {
            p += strlen (cookieName) + 1; while (*p == ' ' || *p == '=' ) p++; // p points to cookie value in HTTP request
            cstring s; while (*p > ' ' && *p != ';') s += *(p ++);
            return s;
        }
    }
    return "";
}

void httpServer_t::webSocket_t::setHttpReplyStatus (const char *status) {
    __httpReplyStatus__ = status;
}

void httpServer_t::webSocket_t::setHttpReplyHeaderField (Cstring<300> fieldName, Cstring<300> fieldValue) {
    __httpReplyHeader__ += fieldName;
    __httpReplyHeader__ +=  + ": ";
    __httpReplyHeader__ += fieldValue;
    __httpReplyHeader__ += "\r\n"; 
}

void httpServer_t::webSocket_t::setHttpReplyCookie (Cstring<300> cookieName, Cstring<300> cookieValue, time_t expires, Cstring<300> path) {
    char e [50] = "";
    if (expires) {
        if (expires < 1600000000) { // 1600000000 ~2020
            cout << ( dmesgQueue << "[httpConn] " << "can't set cookie expiration time for the time has not been set yet" );
        }
        struct tm st;
        gmtime_r (&expires, &st);
        strftime (e, sizeof (e), "; Expires=%a, %d %b %Y %H:%M:%S GMT", &st);
    }
    // save whole fieldValue into cookieName to save stack space
    cookieName += "=";
    cookieName += cookieValue;
    cookieName += "; Path=";
    cookieName += path;
    cookieName += e;
    setHttpReplyHeaderField ("Set-Cookie", cookieName);
}

int httpServer_t::webSocket_t::recvBlock (void *buf, size_t len) {
    // TODO: implement
    return -1;
}

bool httpServer_t::webSocket_t::sendBlock (void *buf, size_t len) {
    // TODO: implement
    return false;
}

bool httpServer_t::webSocket_t::recvString (char *buf, size_t len) {
    // TODO: implement
    return false;
}

bool httpServer_t::webSocket_t::sendString (const char *buf) {
    // TODO: implement
    return false;
}

int httpServer_t::webSocket_t::peek () {
    // TODO: implement
    return 0;
}

bool httpServer_t::webSocket_t::__sendFrame__ (byte *buffer, size_t bufferSize, byte frameType) {
    // TODO: implement
    return false;
}


char* httpServer_t::webSocket_t::stristr (const char *haystack, const char *needle) { 
    if (!haystack || !needle) return NULL; // nonsense
    int n = strlen (haystack) - strlen (needle) + 1;
    for (int i = 0; i < n; i++) {
        int j = i;
        int k = 0;
        char hChar, nChar;
        bool match = true;
        while (*(needle + k)) {
            hChar = *(haystack + j); if (hChar >= 'a' && hChar <= 'z') hChar -= 32; // convert to upper case
            nChar = *(needle + k); if (nChar >= 'a' && nChar <= 'z') nChar -= 32; // convert to upper case
            if (hChar != nChar && nChar) {
                match = false;
                break;
            }
            j ++;
            k ++;
        }
        if (match) return (char *) haystack + i; // match!
    }
    return NULL; // no match
}

void httpServer_t::webSocket_t::__runConnectionTask__ () {

    // ----- this is where HTTP connection starts, the socket is already opened -----

    bool keepAlive;
    do { // while keepAlive

        // 1. read the request
        switch (tcpConnection_t::recvString (__httpRequestAndReplyBuffer__, HTTP_BUFFER_SIZE, "\r\n\r\n")) {
            case -1:                // error
            case 0:                 // connection closed by peer
                                    return;
            case HTTP_BUFFER_SIZE:  // buffer full but end of request did not arrive
                                    cout << ( dmesgQueue << "[httpConn] " << "buffer too small for " << __httpRequestAndReplyBuffer__ );
                                    tcpConnection_t::sendString (reply507);
                                    return;
            default:                break; // continue
        }

        // connection: keep-alive?
        keepAlive = (strstr (__httpRequestAndReplyBuffer__, "Connection: keep-alive") != NULL);

        // 2. is it a websocket request?
        if (strstr (__httpRequestAndReplyBuffer__, "Upgrade: websocket")) {

            // a. change timeout for WebSocket
            setIdleTimeout (HTTP_CONNECTION_WS_TIME_OUT);

            // b. get additional memory for frame buffers
            __recvFrameBuffer__ = (byte *) malloc (2 * HTTP_WS_FRAME_MAX_SIZE);
            if (!__recvFrameBuffer__) {
                cout << ( dmesgQueue << (const char *) "[httpConn] " << "out of memory" );
                return;                                  
            }
            __sendFrameBuffer__ = __recvFrameBuffer__ + HTTP_WS_FRAME_MAX_SIZE;

            // c. do the handshake with the browser so it would consider webSocket connection established
            char *i = strstr (__httpRequestAndReplyBuffer__, "Sec-WebSocket-Key: ");
            if (i) {             
                i += 19;
                char *j = strstr (i, "\r\n");
                if (j) {
                    if (j - i <= 24) { // Sec-WebSocket-Key is not supposed to exceed 24 characters
                        char key [25]; 
                        memcpy (key, i, j - i); 
                        key [j - i] = 0; 
                        // calculate Sec-WebSocket-Accept
                        char s1 [64]; 
                        strcpy (s1, key); 
                        strcat (s1, (char *) "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"); 
                        #define SHA1_RESULT_SIZE 20
                        unsigned char s2 [SHA1_RESULT_SIZE]; 
                        esp_sha (SHA1, (unsigned char *) s1, strlen (s1), s2);
                        #define WS_CLIENT_KEY_LENGTH 24
                        size_t olen = WS_CLIENT_KEY_LENGTH;
                        char s3 [32];
                        mbedtls_base64_encode ((unsigned char *) s3, 32, &olen, s2, SHA1_RESULT_SIZE);
                        // compose websocket accept response and send it back to the client
                        char buffer  [255]; // 255 will do
                        sprintf (buffer, "HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", s3);
                        if (tcpConnection_t::sendString (buffer) <= 0)
                            return;
                    } else { // |key| > 24
                        cout << ( dmesgQueue << "[webSocket] WsRequest key too long" );
                        return;
                    }
                } else { // j == NULL
                    cout << ( dmesgQueue << "[webSocket] WsRequest without key" );
                    return;
                }

                // d. call __wsRequestHandlerCallback__
                if (__wsRequestHandlerCallback__) {
                    __wsRequestHandlerCallback__ (__httpRequestAndReplyBuffer__, this);
                } else {
                    cout << ( dmesgQueue << "[httpConn] " << "wsRequestHandlerCallback was not provided to handle WebSocket" );
                    return;
                }

            } else { // i == NULL
                cout << ( dmesgQueue << "[webSocket] WsRequest without key" );
            }
            return;
        }

        // 3. will httpRequestHandlerCallback function provide (content part of) the reply?
        String httpReplyContent ("");
        if (__httpRequestHandlerCallback__) {
            httpReplyContent = __httpRequestHandlerCallback__ (__httpRequestAndReplyBuffer__, (httpConnection_t *) this);
            if (!httpReplyContent) { // out of memory
                cout << ( dmesgQueue << "[httpConn] " << "out of memory" );
                tcpConnection_t::sendString (reply503);
                return;
            }
        }

        if (httpReplyContent != "") {

            // if content-type was not provided with __httpRequestHandlerCallback__ try guessing what it is
            if (!strstr (__httpReplyHeader__, "Content-Type")) { 
                if (stristr ((char *) httpReplyContent.c_str (), "<HTML")) {
                    setHttpReplyHeaderField ("Content-Type", "text/html");
                } else if (strstr ((char *) httpReplyContent.c_str (), "{")) {
                    setHttpReplyHeaderField ("Content-Type", "application/json");
                // ... add more if needed
                } else {
                    setHttpReplyHeaderField ("Content-Type", "text/plain");
                }
            }
            if (__httpReplyHeader__.errorFlags ()) {
                cout << ( dmesgQueue << "[httpConn] " << "header reply buffer too small" );
                tcpConnection_t::sendString (reply507);
                return;
            }

            // construct the whole HTTP reply from differrent pieces and send it to the client (browser)
            // if the reply is short enough send it in one block, 
            // if not send header first and then the content, so we won't have to move hugh blocks of data
            __httpRequestAndReplyBuffer__ = "";
            __httpRequestAndReplyBuffer__ += "HTTP/1.1 ";
            __httpRequestAndReplyBuffer__ += __httpReplyStatus__.c_str ();
            __httpRequestAndReplyBuffer__ += "\r\n";
            __httpRequestAndReplyBuffer__ += __httpReplyHeader__.c_str ();
            __httpRequestAndReplyBuffer__ += "Content-Length: ";
            __httpRequestAndReplyBuffer__ += httpReplyContent.length ();
            __httpRequestAndReplyBuffer__ += "\r\n\r\n";

            unsigned long httpReplyHeaderLen = __httpRequestAndReplyBuffer__.length ();

            // can not happen due to the difference in lengths, we can skip checking:
            // if (ths->__httpRequestAndReplyBuffer__.error ()) {
            //    dmesg ("[httpConn] __httpRequestAndReplyBuffer__ too small");
            //    sendAll (ths->__connectionSocket__, reply507, HTTP_CONNECTION_TIME_OUT);
            //    goto endOfConnection;
            // }

            unsigned long contentBytesToSendThisTime = min ((unsigned long) httpReplyContent.length (), HTTP_BUFFER_SIZE - httpReplyHeaderLen);
            unsigned long contentBytesSentTotal = 0;
            memcpy (&__httpRequestAndReplyBuffer__ [httpReplyHeaderLen], (char *) httpReplyContent.c_str (), contentBytesToSendThisTime);
            while (true) {
                if (tcpConnection_t::sendBlock (__httpRequestAndReplyBuffer__, httpReplyHeaderLen + contentBytesToSendThisTime) <= 0)
                    return;

                httpReplyHeaderLen = 0; // we won't send the header any more
                contentBytesSentTotal += contentBytesToSendThisTime;
                contentBytesToSendThisTime = min (httpReplyContent.length () - contentBytesSentTotal, (unsigned long) HTTP_BUFFER_SIZE);
                if (!contentBytesToSendThisTime)
                    break;
                memcpy (__httpRequestAndReplyBuffer__, (char *) httpReplyContent.c_str () + contentBytesSentTotal, contentBytesToSendThisTime);
            }

            // content-type provided with __httpRequestHandlerCallback__ was sent to the browser, continue with the next request on this connection
            goto nextHttpRequest;
        }

        // 4. try to process the request as a file name
        if (__replyWithFileContentPtr__)
            if ((this->*__replyWithFileContentPtr__) ())
                goto nextHttpRequest;

        // 5. HTTP request was not handeled above
        if (tcpConnection_t::sendString (reply404) <= 0)
            return;

    nextHttpRequest:
        // check how much stack did we use
        UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
        if (__lastHighWaterMark__ > highWaterMark) {
            cout << ( dmesgQueue << "[httpConn] " << "new HTTP connection stack high water mark reached: " << highWaterMark << " not used bytes" );
            __lastHighWaterMark__ = highWaterMark;
        }

        // if we are running out of ESP32's resources we won't try to keep the connection alive, this would slow down the server a bit but it would let still it handle requests from different clients
        if (getSocket () >= LWIP_SOCKET_OFFSET + MEMP_NUM_NETCONN - 2)  // running out of sockets
            return;
        if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) < 2 * HTTP_CONNECTION_STACK_SIZE) // there is not a memory block large enough evailable to start 2 new tasks that would handle the connection
            return;

        // restore default values of member variables for the next HTTP request on this connection                              
        __httpRequestAndReplyBuffer__ = "";
        __httpReplyStatus__ = "200 OK";
        __httpReplyHeader__ = "";
    
    } while (keepAlive); // while

}




// ----- httpServer_t::httpConnection_t (inherits everything from webSocket_t, no extra implementation needed here) -----



// ----- httpServer_t implementation -----

httpServer_t::httpServer_t (String (*httpRequestHandlerCallback) (const char *httpRequest, httpServer_t::httpConnection_t *hcn),
                            void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck),
                            int serverPort,
                            bool (*firewallCallback) (char *clientIP, char *serverIP),
                            bool runListenerInItsOwnTask
                           ) : tcpServer_t (serverPort, firewallCallback, runListenerInItsOwnTask),
                               __httpRequestHandlerCallback__ (httpRequestHandlerCallback),
                               __wsRequestHandlerCallback__ (wsRequestHandlerCallback) {

}

tcpConnection_t *httpServer_t::__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) {

    webSocket_t *connection = new (std::nothrow) webSocket_t (__fileSystem__, __replyWithFileContentPtr__, connectionSocket, clientIP, serverIP, __httpRequestHandlerCallback__, __wsRequestHandlerCallback__);
    if (!connection) {
        cout << ( dmesgQueue << "[httpServer] " << "can't create connection instance, out of memory" );
        send (connectionSocket, reply503, strlen (reply503), 0);
        close (connectionSocket); // normally tcpConnection would do this but if it is not created we have to do it here since the connection was not created
        return NULL;
    }

    connection->setIdleTimeout (HTTP_CONNECTION_TIME_OUT);


    #define tskNORMAL_PRIORITY (tskIDLE_PRIORITY + 1)
    if (pdPASS != xTaskCreate ([] (void *thisInstance) {
                                                            httpConnection_t* ths = (httpConnection_t *) thisInstance; // get "this" pointer

                                                            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                                                __runningTcpConnections__ ++;
                                                            xSemaphoreGive (getLwIpMutex ());

                                                            ths->__runConnectionTask__ ();

                                                            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                                                __runningTcpConnections__ --;
                                                            xSemaphoreGive (getLwIpMutex ());

                                                            delete ths;
                                                            vTaskDelete (NULL); // it is connection's responsibility to close itself
                                                        }
                                , "httpConn", HTTP_CONNECTION_STACK_SIZE, connection, tskNORMAL_PRIORITY, NULL)) {

        cout << ( dmesgQueue << "[httpServer] " << "can't create connection task, out of memory" );
        send (connectionSocket, reply503, strlen (reply503), 0);
        delete (connection); // normally tcpConnection would do this but if it is not running we have to do it here
        return NULL;
    }

    return NULL; // success, but don't return connection, since it may already been closed and destructed by now
}


