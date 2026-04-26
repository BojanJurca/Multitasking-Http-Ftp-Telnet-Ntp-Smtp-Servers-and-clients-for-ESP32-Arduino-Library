/*

    httpServer.h
  
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


#pragma once
#ifndef __HTTP_SERVER__
    #define __HTTP_SERVER__


    #include <WiFi.h>
    #include <dmesg.hpp>
    #include <Cstring.hpp>
    #include <sha/sha_parallel_engine.h>
    #include <mbedtls/base64.h>
    #include <mbedtls/md.h>
    #include "tcpServer.h"
    #include "tcpConnection.h"


    // ----- TUNING PARAMETERS -----

    #define HTTP_CONNECTION_STACK_SIZE (5 * 1024 + 512)
    #define HTTP_BUFFER_SIZE 1440 // optimal TCP payload: MTU = 1500 bytes, TCP header = 20 bytes, IPv6 header = 40 bytes: 1500 - 20 - 40 = 1440
    #define HTTP_CONNECTION_TIME_OUT 3
    #define HTTP_REPLY_STATUS_MAX_LENGTH 32
    #define HTTP_REPLY_HEADER_MAX_LENGTH 300
    #define HTTP_WS_FRAME_MAX_SIZE 1440
    #define HTTP_CONNECTION_WS_TIME_OUT 300

    // ----- REPLIES -----

    #define reply400 "HTTP/1.0 400 Bad request\r\nConnection: close\r\nContent-Length:34\r\n\r\nFormat of HTTP request is invalid."
    #define reply404 "HTTP/1.0 404 Not found\r\nConnection: close\r\nContent-Length:10\r\n\r\nNot found."
    #define reply503 "HTTP/1.0 503 Service unavailable\r\nConnection: close\r\nRetry-After: 3\r\nContent-Length:40\r\n\r\nHTTP service is not available right now."
    #define reply507 "HTTP/1.0 507 Insuficient storage\r\nConnection: close\r\nContent-Length:41\r\n\r\nThe buffers of HTTP server are too small."


    // WebSocket frame types
    static constexpr byte STRING_FRAME_TYPE = 1;
    static constexpr byte BINARY_FRAME_TYPE = 2;
    static constexpr byte CLOSE_FRAME_TYPE  = 8;


    class httpServer_t : public tcpServer_t {

        public:
            class httpConnection_t;

            class webSocket_t : public tcpConnection_t {

                friend httpServer_t;

                private:
                    void *__fileSystem__ = NULL;
                    bool (webSocket_t::*__replyWithFileContentPtr__) () = NULL;

                    String (*__httpRequestHandlerCallback__) (const char *httpRequest, httpConnection_t *hcn);
                    void (*__wsRequestHandlerCallback__) (const char *httpRequest, webSocket_t *webSck);

                    static UBaseType_t __lastHighWaterMark__;

                    // HTTP connection related variables
                    Cstring<HTTP_BUFFER_SIZE> __httpRequestAndReplyBuffer__; // empty by default
                    Cstring<HTTP_REPLY_STATUS_MAX_LENGTH> __httpReplyStatus__ = "200 OK"; // by default
                    Cstring<HTTP_REPLY_HEADER_MAX_LENGTH> __httpReplyHeader__; // empty by default

                    // WebSocket related variables
                    byte *__recvFrameBuffer__ = NULL;
                    byte *__sendFrameBuffer__ = NULL;
                    int __bytesReceived__;
                    byte *__frameMask__;
                    int __payloadLength__;
                    byte *__payload__; // will be initialized when the header is read

                    // WebSocket functions

                    enum RECV_FRAME_STATE {
                        EMPTY = 0,                                  // frame is empty
                        READING_SHORT_HEADER = 1,
                        READING_MEDIUM_HEADER = 2,
                        READING_PAYLOAD = 3,
                        FULL = 4                                    // frame is full and waiting to be read by the calling program
                    } __recvFrameState__ = EMPTY; 

                    // Internal helpers (implemented in .cpp)
                    bool __sendFrame__ (byte *buffer, size_t bufferSize, byte frameType);
                    static char *stristr (const char *haystack, const char *needle);
                    void __runConnectionTask__ ();
                    #ifdef __THREAD_SAFE_FS__
                        bool __replyWithFileContent__ ();
                    #endif

                public:
                    webSocket_t (void *FileSystem,
                                 bool (webSocket_t::*replyWithFileContentPtr) (),
                                 int connectionSocket,
                                 char *clientIP,
                                 char *serverIP,
                                 String (*httpRequestHandlerCallback) (const char *httpRequest, httpConnection_t *hcn),
                                 void (*wsRequestHandlerCallback) (const char *httpRequest, webSocket_t *webSck));

                    ~webSocket_t ();

                    // HTTP helpers
                    char *getHttpRequest ();
                    Cstring<300> getHttpRequestHeaderField (const char *fieldName);
                    Cstring<300> getHttpRequestCookie (const char *cookieName);
                    void setHttpReplyStatus (const char *status);
                    void setHttpReplyHeaderField (Cstring<300> fieldName, Cstring<300> fieldValue);
                    void setHttpReplyCookie (Cstring<300> cookieName, Cstring<300> cookieValue, time_t expires = 0, Cstring<300> path = "/");

                    // WebSocket I/O
                    int recvBlock (void *buf, size_t len);
                    bool sendBlock (void *buf, size_t len);
                    bool recvString (char *buf, size_t len);
                    bool sendString (const char *buf);
                    int peek ();
            };

            /*
                This is rather unusual class hierarhy. Normally webSocket calss would inherit from httpConnection and add some additional functions to it but here is just the other way arround.
                The reason for this is that once the instance (say httpConnection) is already created we can not extend it to its inherited class (say webSocket), but we can always do the
                other way arround. The implementation is easier this way.
            */

            class httpConnection_t : public webSocket_t {
                using tcpConnection_t::recvBlock;
                using tcpConnection_t::sendBlock;
                using tcpConnection_t::recvString;
                using tcpConnection_t::sendString;
                using tcpConnection_t::peek;
            };


    private:

        void *__fileSystem__ = NULL;
        bool (webSocket_t::*__replyWithFileContentPtr__) () = NULL;
        
        String (*__httpRequestHandlerCallback__) (const char *httpRequest, httpConnection_t *hcn);
        void (*__wsRequestHandlerCallback__) (const char *httpRequest, webSocket_t *webSck);


    public: 

        httpServer_t (String (*httpRequestHandlerCallback) (const char *httpRequest, httpConnection_t *hcn),
                      void (*wsRequestHandlerCallback) (const char *httpRequest, webSocket_t *webSck) = NULL,
                      int serverPort = 80,
                      bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                      bool runListenerInItsOwnTask = true);

        #ifdef __THREAD_SAFE_FS__
            // this part will not compile with .cpp

            // constructor with a file system
            httpServer_t (threadSafeFS::FS& fileSystem,
                          String (*httpRequestHandlerCallback) (const char *httpRequest, httpConnection_t *hcn) = NULL,
                          void (*wsRequestHandlerCallback) (const char *httpRequest, webSocket_t *webSck) = NULL,
                          int serverPort = 80,
                          bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                          bool runListenerInItsOwnTask = true) : tcpServer_t (serverPort, firewallCallback, runListenerInItsOwnTask),
                                                                 __fileSystem__ (&fileSystem),
                                                                 __httpRequestHandlerCallback__ (httpRequestHandlerCallback),
                                                                 __wsRequestHandlerCallback__ (wsRequestHandlerCallback) {
                if (__fileSystem__) {
                    // get address of a function that handles files
                    __replyWithFileContentPtr__ = &webSocket_t::__replyWithFileContent__;

                    // create directory structure and default index.html
                    if (!fileSystem.isDirectory ("/var/www/html")) {
                        fileSystem.mkdir ("/var");
                        fileSystem.mkdir ("/var/www");
                        fileSystem.mkdir ("/var/www/html");
                        if (!fileSystem.isDirectory ("/var/www/html"))
                            cout << ( dmesgQueue << "[httpServer] " "can't create /var/www/html" );
                    }
                    if (!fileSystem.isFile ("/var/www/html/index.html")) {
                        threadSafeFS::File f = fileSystem.open ("/var/www/html/index.html", "w");
                        if (f) {
                            f.print ("<!DOCTYPE html>\n"
                                     "<html lang='en'>\n"
                                     "   <head>\n"
                                     "      <meta charset='UTF-8'>\n"
                                     "      <title>Hello world!</title>\n"
                                     "   </head>\n"
                                     "   <body>\n"
                                     "      <h1>Hello world!</h1>\n"
                                     "   </body>\n"
                                     "</html>");
                            f.close ();
                        }
                    }

                }
            }

        #endif

        tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) override;

        // accept any connection, the client will get notified in __createConnectionInstance__
        inline tcpConnection_t *accept () __attribute__((always_inline)) { 
            if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) < HTTP_CONNECTION_STACK_SIZE) { 
                // There is not a memory block large enough evailable to start new task that would handle the new connection.
                // If we ::accept () the connection now we would only have to report503 "HTTP/1.0 503 Service unavailable
                // to the client later. But if we don't call ::accept () now the incoming connection will wait for a while,
                // and perhaps get ::acceptted () a few momemnts later 
                return NULL;
            } else {
                return tcpServer_t::accept (); 
            }
        }

    };


    #ifdef __THREAD_SAFE_FS__
        // this part will not compile with .cpp

        // called only from __runConnectionTask__ ()
        bool httpServer_t::webSocket_t::__replyWithFileContent__ () {
            threadSafeFS::FS *fileSystem = (threadSafeFS::FS *) __fileSystem__;
            if (!fileSystem) return false; // shouldn't happen but check anyway

            bool browserAcceptsGzip = (getHttpRequestHeaderField ("Accept-Encoding").indexOf ("gzip") >= 0);

            if (fileSystem->mounted ()) {
                if (strstr (__httpRequestAndReplyBuffer__.c_str (), "GET ") == __httpRequestAndReplyBuffer__.c_str ()) {
                    char *p = strstr (__httpRequestAndReplyBuffer__.c_str () + 4, " ");
                    if (p) {
                        *p = 0;
                        // get file name from HTTP request
                        Cstring<300>fileName = *(__httpRequestAndReplyBuffer__.c_str () + 4) == '/' ? (__httpRequestAndReplyBuffer__.c_str () + 5) : (__httpRequestAndReplyBuffer__.c_str () + 4);
                        if (fileName == "") fileName = "index.html";
                        fileName = Cstring<300>("/var/www/html/") + fileName.c_str ();

                        // if Content-type was not provided in __httpRequestHandlerCallback__ try guessing what it is
                        if (!strstr (__httpReplyHeader__.c_str (), "Content-Type")) { 
                                    if (fileName.endsWith (".bmp"))                                        setHttpReplyHeaderField ("Content-Type", "image/bmp");
                            else if (fileName.endsWith (".css"))                                        setHttpReplyHeaderField ("Content-Type", "text/css");
                            else if (fileName.endsWith (".csv"))                                        setHttpReplyHeaderField ("Content-Type", "text/csv");
                            else if (fileName.endsWith (".gif"))                                        setHttpReplyHeaderField ("Content-Type", "image/gif");
                            else if (fileName.endsWith (".htm") || fileName.endsWith (".html"))         setHttpReplyHeaderField ("Content-Type", "text/html");
                            else if (fileName.endsWith (".htm.gz") || fileName.endsWith (".html.gz"))   setHttpReplyHeaderField ("Content-Type", "text/html");
                            else if (fileName.endsWith (".jpg") || fileName.endsWith (".jpeg"))         setHttpReplyHeaderField ("Content-Type", "image/jpeg");
                            else if (fileName.endsWith (".js"))                                         setHttpReplyHeaderField ("Content-Type", "text/javascript");
                            else if (fileName.endsWith (".json"))                                       setHttpReplyHeaderField ("Content-Type", "application/json");
                            else if (fileName.endsWith (".mpeg"))                                       setHttpReplyHeaderField ("Content-Type", "video/mpeg");
                            else if (fileName.endsWith (".pdf"))                                        setHttpReplyHeaderField ("Content-Type", "application/pdf");
                            else if (fileName.endsWith (".png"))                                        setHttpReplyHeaderField ("Content-Type", "image/png");
                            else if (fileName.endsWith (".tif") || fileName.endsWith (".tiff"))         setHttpReplyHeaderField ("Content-Type", "image/tiff");
                            else if (fileName.endsWith (".txt"))                                        setHttpReplyHeaderField ("Content-Type", "text/plain");
                            // ... add more if needed but Contet-Type can often be omitted without problems ...
                        }

                        threadSafeFS::File f;
                        // check if there is a compressed .gz file available
                        if (fileName.endsWith (".gz")) {
                            // browser requested a .gz file
                            setHttpReplyHeaderField ("Content-Encoding", "gzip");
                            f = fileSystem->open (fileName, "r");
                        } else {

                            // if the browser accepts gzip
                            if (browserAcceptsGzip) {
                                if (fileSystem->exists (fileName + ".gz")) {
                                    fileName += ".gz";
                                    // browser accepts .gz and .gz exists
                                    f = fileSystem->open (fileName, "r");
                                    if (f) {
                                        // .gz opened
                                        setHttpReplyHeaderField ("Content-Encoding", "gzip");
                                    } else {
                                        // no, try opening uncompressed fileName
                                        f = fileSystem->open (fileName, "r");
                                    }
                                } else {
                                    // browser accepts .gz but it doesn't exist, open uncompressed file
                                    f = fileSystem->open (fileName, "r");
                                }
                            } else {
                                // browser doesn't accept .gz, open uncompressed fileName
                                f = fileSystem->open (fileName, "r");
                            }
                        }

                        if (f && !f.isDirectory ()) {                               
                            if (__httpReplyHeader__.errorFlags ()) {
                                f.close ();
                                cout << ( dmesgQueue << "[httpConn] " "reply header buffer too small" );
                                tcpConnection_t::sendString (reply507);
                                return true;
                            }

                            // construct the whole HTTP reply from differrent pieces and send it to client (browser)
                            // if the reply is short enough send it in one block,
                            // if not send header first and then the content, so we won't have to move hughe blocks of data
                            unsigned long httpReplyContentLen = f.size ();
                            __httpRequestAndReplyBuffer__ = "";
                            __httpRequestAndReplyBuffer__ += "HTTP/1.1 ";
                            __httpRequestAndReplyBuffer__ += __httpReplyStatus__.c_str ();
                            __httpRequestAndReplyBuffer__ += "\r\n";
                            __httpRequestAndReplyBuffer__ += __httpReplyHeader__.c_str ();
                            __httpRequestAndReplyBuffer__ += "Content-Length: ";
                            __httpRequestAndReplyBuffer__ += httpReplyContentLen; 
                            __httpRequestAndReplyBuffer__ += "\r\n\r\n";

                            unsigned long httpReplyHeaderLen = __httpRequestAndReplyBuffer__.length ();

                            // can not happen due to the difference in lengths, we can skip checking:
                            // if (ths->__httpRequestAndReplyBuffer__.error ()) {
                            //    dmesg ("[httpConn] __httpRequestAndReplyBuffer__ too small");
                            //    sendAll (ths->__connectionSocket__, reply507, HTTP_CONNECTION_TIME_OUT);
                            //    goto endOfConnection;
                            // }

                            int bytesToReadThisTime = min (httpReplyContentLen, HTTP_BUFFER_SIZE - httpReplyHeaderLen); 
                            int bytesReadThisTime = f.read ((uint8_t *) &__httpRequestAndReplyBuffer__ [httpReplyHeaderLen], bytesToReadThisTime);

                            __httpRequestAndReplyBuffer__ [HTTP_BUFFER_SIZE] = 0;
                            if (tcpConnection_t::sendBlock (__httpRequestAndReplyBuffer__, httpReplyHeaderLen + bytesReadThisTime) <= 0) {
                                f.close ();
                                cout << ( dmesgQueue << "[httpConn] " "send failed" );
                                return true;
                            }
                    
                            int bytesSent = bytesReadThisTime; // already sent
                            while (bytesSent < httpReplyContentLen) {
                                bytesToReadThisTime = min (httpReplyContentLen - bytesSent, (unsigned long) HTTP_BUFFER_SIZE);
                                bytesReadThisTime = f.read ((uint8_t *) &__httpRequestAndReplyBuffer__ [0], (unsigned long) bytesToReadThisTime);

                                delay (10); // WiFi STAtion sometimes disconnects at heavy load - maybe giving it some time would make things better? 
                                if (!bytesReadThisTime) {
                                    f.close ();
                                    cout << ( dmesgQueue << "[httpConn] " "can't read " << fileName );
                                    return true;
                                }
                
                                if (tcpConnection_t::sendBlock (__httpRequestAndReplyBuffer__, bytesReadThisTime) <= 0) {
                                    f.close ();
                                    cout << ( dmesgQueue << "[httpConn] " "can't send " << fileName );
                                    return true;
                                }
                                bytesSent += bytesReadThisTime; // already sent
                            }
                            // HTTP reply sent
                            f.close ();                                   
                            return true;

                        } // if file is open

                    }
                }
            }

            return false;
        }

    #endif

#endif
