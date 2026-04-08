/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino/blob/master/PROJECT_STATE.md
*/


#include <WiFi.h>
#include <httpServer.h>

// A callback function that would handle HTTP requests, it returns the content part of HTTP reply, the header will be added later by the HTTP server
String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) {

  // Must be reentrant !!!


  #define httpRequestIs(X) (strstr(httpRequest,X)==httpRequest)

  if (httpRequestIs ("GET / ") || httpRequestIs ("GET /index.html "))
    return  "<!DOCTYPE html>\n"
            "<html lang='en'>\n"
            "   <head>\n"
            "      <meta charset='UTF-8'>\n"
            "      <title>Hello world!</title>\n"
            "   </head>\n"
            "   <body>\n"
            "      <h1>Hello world!</h1>\n"
            "   </body>\n"
            "</html>";

  // let the HTTP server hanle the request itself
  return "";
}

httpServer_t *httpServer = NULL;

void setup () {
  Serial.begin (115200);

  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 1️⃣ Create HTTP server instance without listener's task (runListenerInItsOwnTask = false)
  // HTTP server's listener would have to host in the loop task in this case, 
  // not having a seperate listening task would save 3 KB of memory
                                                                            // optional arguments:
                                                                            // threadSafeFS::FS& fileSystem,
  httpServer = new (std::nothrow) httpServer_t (httpRequestHandlerCallback, // String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) = NULL, 
                                                NULL,                       // void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                                80,                         // int serverPort = 80,
                                                NULL,                       // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                                false);                     // bool runListenerInItsOwnTask = true
                                                                              

  // check if FTP server instance is created && FTP server is running
  if (httpServer && *httpServer)
    Serial.println ("HTTP server started");
  else
    Serial.println ("HTTP server did not start");


 // Use web browser to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000); 
      Serial.println ("   ."); 
  } 
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());


  // ...
}

void loop () {


  // 2️⃣  Periodically call accept
  if (httpServer)
    httpServer->accept ();


    // ... add your own code - it shoud not block for too long
}
