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


// 1️⃣ provide a firewall callback function to HTTP server that would tell which connctions to accept and which to refuse
bool firewallCallback (char *clientIP, char *serverIP) { 

  // Must be reentrant !!

  // accept only connections from local network, for example 10.18.1.*
  if (strstr (clientIP, "10.18.1.") == clientIP)
    return true;
  else 
    return false; 
}


void setup () {
  Serial.begin (115200);

  // Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 2️⃣ create HTTP server instance with firewall callback function
  httpServer = new (std::nothrow) httpServer_t (httpRequestHandlerCallback, // optional arguments:
                                                NULL,                       // void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                                80,                         // int serverPort = 80,
                                                firewallCallback);          // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                                                            // bool runListenerInItsOwnTask = true


  // Check if HTTP server instance is created && HTTP server is running
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

}
