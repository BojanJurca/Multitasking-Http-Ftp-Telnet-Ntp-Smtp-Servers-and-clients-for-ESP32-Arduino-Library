/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
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


void setup () {
  Serial.begin (115200);

  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");

  // Use web browser to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000); 
      Serial.println ("   ."); 
  } 
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());


  // ... your code here
}

void loop () {


  // 1️⃣ Create static (so it would contiune to run even when loop finishes) HTTP server instance
                                                              // Optional arguments (when file system is not included):
  static httpServer_t httpServer (httpRequestHandlerCallback, // String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) = NULL, 
                                  NULL,                       // void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                  80,                         // int serverPort = 80,
                                  NULL,                       // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                  false);                     // SET THIS ARGUMENT TO FALSE: bool runListenerInItsOwnTask = true
  // this would spare roughly 3 KB of memory that listener task would use otherwise                                   

  /* Check if HTTP server is running
  if (httpServer)
    Serial.println ("HTTP server started");
  else
    Serial.println ("HTTP server did not start");
  */


  // 2️⃣ Periodically call accept yourself since the listener is not running (in its own task)
  httpServer.accept ();


  // ... your code here - should not block for too long - this would also block calling accept ()
}
