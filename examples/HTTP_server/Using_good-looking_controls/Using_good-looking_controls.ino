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


  // variables that hold server-side state
  static char mySwitch [6] = "false"; // "false" or "true"
  static int mySlider = 6; // 0 .. 10
  static char myRadioButtons [10] = "option1"; // "option1" or "option2"


  // 1️⃣ REST API interface

  // browser wants to read mySwitch state
   if (httpRequestIs ("GET /mySwitch "))                {
                                                          return "{\"id\":\"mySwitch\",\"value\":\"" + String (mySwitch) + "\"}";
                                                        }
  // browser wants to set mySwitch state               
  else if (httpRequestIs ("PUT /mySwitch/"))            {
                                                          strncpy (mySwitch, httpRequest + 14, sizeof (mySwitch));
                                                          mySwitch [sizeof (mySwitch) - 1] = 0;
                                                          char *p = strchr (mySwitch, ' '); if (p) *p = 0;
                                                          return "{\"id\":\"mySwitch\",\"value\":\"" + String (mySwitch) + "\"}";
                                                        }

  // browser wants to read mySlider value
  else if (httpRequestIs("GET /mySlider "))             {
                                                          return "{\"id\":\"mySlider\",\"value\":\"" + String (mySlider) + "\"}";
                                                        }
  // browser wants to set mySlider value                                                        
  else if (httpRequestIs ("PUT /mySlider/"))            {
                                                          mySlider = atoi (httpRequest + 14);
                                                          return "{\"id\":\"mySlider\",\"value\":\"" + String (mySlider) + "\"}";
                                                        }

  // browser wants to read myRadioButtons state
  else if (httpRequestIs ("GET /myRadioButtons "))      { 
                                                          return "{\"id\":\"myRadioButtons\",\"value\":\"" + String (myRadioButtons) + "\"}";
                                                        }
  // browser wants to set myRadioButtons state
  else if (httpRequestIs ("PUT /myRadioButtons/"))      {
                                                          strncpy (myRadioButtons, httpRequest + 18, sizeof (myRadioButtons));
                                                          myRadioButtons [sizeof (myRadioButtons) - 1] = 0;
                                                          char *p = strchr (myRadioButtons, ' '); if (p) *p = 0;
                                                          return "{\"id\":\"myRadioButtons\",\"value\":\"" + String (myRadioButtons) + "\"}";
                                                        }

  // browser sent request about pressing a button
  else if (httpRequestIs ("PUT /blueButton/pressed "))  {
                                                          return "{\"id\":\"blueButton\",\"value\":\"pressed\"}";
                                                        }
  else if (httpRequestIs ("PUT /greenButton/pressed ")) {
                                                          return "{\"id\":\"greenButton\",\"value\":\"pressed\"}";
                                                        }
  else if (httpRequestIs ("PUT /redButton/pressed "))   {
                                                          return "{\"id\":\"redButton\",\"value\":\"pressed\"}";
                                                        }


  // 2️⃣ HTML page that will be sent to the browser - Javascript uses REST API interface
  if (httpRequestIs ("GET / ") || httpRequestIs ("GET /index.html ")) {
    #include "clientPage.h"
    return clientPage;
  }

  // Let the httpServer handle the request itself
  return "";
}

httpServer_t *httpServer = NULL;

void setup () {
  Serial.begin (115200);

  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");

  // Create HTTP server instance passing it callback function that will handle the HTTP requests 
                                                                            // optional arguments:
                                                                            // threadSafeFS::FS& fileSystem,
  httpServer = new (std::nothrow) httpServer_t (httpRequestHandlerCallback);// String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) = NULL,
                                                                            // void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                                                            // int serverPort = 80,
                                                                            // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
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
