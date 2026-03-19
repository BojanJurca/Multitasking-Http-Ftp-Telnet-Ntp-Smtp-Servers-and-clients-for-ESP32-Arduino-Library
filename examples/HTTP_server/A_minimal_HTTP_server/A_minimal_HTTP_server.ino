#include <WiFi.h>
#include <httpServer.h>


// 1️⃣ A callback function that would handle HTTP requests, it returns the content part of HTTP reply, the header will be added later by the HTTP server
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


  // 2️⃣ Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 3️⃣ Create HTTP server instance passing it callback function that will handle the HTTP requests 
  httpServer = new (std::nothrow) httpServer_t (httpRequestHandlerCallback);  // optional arguments:
                                                                              // void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                                                              // int serverPort = 80,
                                                                              // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                                                              // bool runListenerInItsOwnTask = true


  // 4️⃣ Check if HTTP server instance is created && HTTP server is running
  if (httpServer && *httpServer)
    Serial.println ("HTTP server started");
  else
    Serial.println ("HTTP server did not start");


  // 5️⃣ Use web browser to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000); 
      Serial.println ("   ."); 
  } 
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());


  // ...
}

void loop () {

}
