/*
  **Note:** Each example demonstrates only a specific feature or use case.
  The complete, fully integrated server solution is available here:
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino


  The HTTPS server uses the WolfSSL library, which can be installed through the
  Arduino IDE or downloaded from GitHub: https://github.com/wolfSSL/wolfssl.

  WolfSSL must be installed and configured to match your ESP32 board and the type
  of TLS certificate you plan to use before compiling this example.

  A sample user_settings.h file is included to demonstrate a recommended configuration
  for at least ESP32-WROOM and ESP32-S2 modules using ECC certificates.

  Additionally, example OpenSSL commands are provided to generate a self‑signed ECC 
  certificate and private key suitable for use with ESP32 


  Please take a look at HTTP server examples. After Wolfssl is confugured
  HTTPS server can be used exactly the same way as the HTTP server. 

*/


#include <WiFi.h>

#include <LittleFS.h>             // Or SPIFFS.h or FFat.h or SD.h ...
#include <threadSafeFS.h>         // Include thread-safe wrapper since LittleFS, FFat and SD file systems are not thread safe


#include <httpsServer.h>


// 1️⃣ Use NTP client to get current time - it will be needed to set web session token expiration time
#include <ntpClient.h>
ntpClient_t ntpClient ("1.si.pool.ntp.org", "2.si.pool.ntp.org", "3.si.pool.ntp.org");


// 2️⃣ Manage HTTP requests
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

    return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning ""
}

void setup () {
  Serial.begin (115200);

  // Start file system under (thread-safe wrapper) tsfs (LittleFS or SPIFFS or FFat or SD)
  tsfs.begin (true);


  // 4️⃣ Provide custom allocators to WolfSSL
  if (wolfSSL_SetAllocators(
          [](size_t sz) -> void* { return heap_caps_malloc (sz, MALLOC_CAP_SPIRAM); },
          [](void* p) -> void { heap_caps_free (p); },
          [](void* p, size_t sz) -> void* { return heap_caps_realloc (p,sz, MALLOC_CAP_SPIRAM); }
      ) != WOLFSSL_SUCCESS) {
      Serial.println ("Error setting PSRAM allocators");
  }


  // Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 5️⃣ Create static (so it would contiune to run even when setup finishes) HTTPS server instance passing it callback function that will handle the HTTP requests 
                                                                  // Optional arguments (when file system is included):
  static httpsServer_t httpsServer (/* tsfs, */                   // threadSafeFS::FS& fileSystem,
                                    httpRequestHandlerCallback);  // String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) = NULL,
                                                                  // void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                                                  // int serverPort = 443,
                                                                  // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                                                  // bool runListenerInItsOwnTask = true

  // Check if HTTPS server instance is created && HTTPS server is running
  if (httpsServer)
    Serial.println ("HTTPS server started");
  else
    Serial.println ("HTTPS server did not start");


  // Use web browser to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000);
      Serial.println ("   .");
  }
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());

  ntpClient.syncTime ();


  // ... your code here
}

void loop () {

}
