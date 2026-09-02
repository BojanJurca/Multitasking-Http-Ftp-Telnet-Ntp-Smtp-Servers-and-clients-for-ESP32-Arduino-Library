/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
*/


#include <WiFi.h>
#include <httpServer.h>

#define LED_BUILTIN 2

// A callback function that would handle HTTP requests, it returns the content part of HTTP reply, the header will be added later by the HTTP server
String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) {

  // Must be reentrant !!!


  #define httpRequestIs(X) (strstr(httpRequest,X)==httpRequest)


  // 1️⃣ REST API interface
  if (httpRequestIs ("GET /led ")) {                                            // input (client -> server) = URL address
    return digitalRead (LED_BUILTIN) ? "{\"led\":\"on\"}" : "{\"led\":\"off\"}";  // output (server -> client) = json
  } else if (httpRequestIs ("PUT /led/on ")) {                                  // input (client -> server) = URL address
    digitalWrite (LED_BUILTIN, 1);
    return "{\"led\":\"on\"}";                                                    // output (server -> client) = json
  } else if (httpRequestIs ("PUT /led/off ")) {                                 // input (client -> server) = URL address
    digitalWrite (LED_BUILTIN, 0);
    return "{\"led\":\"off\"}";                                                   // output (server -> client) = json
  }


  // 2️⃣ HTML page that will be sent to the browser - Javascript uses REST API interface
  if (httpRequestIs ("GET / ") || httpRequestIs ("GET /index.html ")) {
    return  "<!DOCTYPE html>\n"
            "<html lang='en'>\n"
            "   <head>\n"
            "      <meta charset='UTF-8'>\n"
            "      <title>REST API</title>\n"
            "   </head>\n"
            "   <body>\n"
            "      <h1>REST API</h1>\n"
            "		   <br><br>\n"
            "		   Led: <input type='checkbox' disabled id='ledSwitch' onClick='turnLed (this.checked)'>\n"
            "   </body>\n"
            "   <script type='text/javascript'>\n"
            "\n"
            "     // respond to switch\n"
            "		  function turnLed (switchIsOn) {\n"
            "       fetch (switchIsOn ? '/led/on' : '/led/off', {\n"
            "         method: 'PUT'\n"
            "       })\n"
            "       .then (response => response.json ())\n"
            "       .then (data => {\n"
            "         const obj = document.getElementById ('ledSwitch');\n"
            "         obj.checked = (data.led === 'on');\n"
            "       })\n"
            "       .catch (err => {\n"
            "         console.error ('Request failed:', err);\n"
            "       });\n"
            "     }\n"
            "\n"
            "     // read current led state when the page loads\n"
            "     fetch ('/led', {\n"
            "       method: 'GET'\n"
            "     })\n"
            "     .then (response => response.json ())\n"
            "     .then (data => {\n"
            "       const obj = document.getElementById ('ledSwitch');\n"
            "       obj.checked = (data.led === 'on');\n"
            "       obj.disabled = false;\n"            
            "     })\n"
            "     .catch (err => {\n"
            "       console.error ('Request failed:', err);\n"
            "     });\n"            
            "\n"
            "   </script>\n"
            "</html>";
  }

  // Let the httpServer handle the request itself
  return "";
}

httpServer_t *httpServer = NULL;

void setup () {
  Serial.begin (115200);

  // Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");

  // Create HTTP server instance passing it callback function that will handle the HTTP requests 
                                                                            // Optional arguments (when file system is not included):
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


  pinMode (LED_BUILTIN, INPUT | OUTPUT);


  // ... your code here
}

void loop () {

}
