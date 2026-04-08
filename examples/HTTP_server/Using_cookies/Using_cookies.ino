/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino/blob/master/PROJECT_STATE.md
*/


#include <WiFi.h>
#include <httpServer.h>
#include <ntpClient.h>            // Setting the time is necessary to set cookie expiration time


String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) {

  // Must be reentrant !!!


  #define httpRequestIs(X) (strstr(httpRequest,X)==httpRequest)

  if (httpRequestIs ("GET / ") || httpRequestIs ("GET /index.html ")) {


    // 1️⃣ Read the cookie from HTTP request
    Cstring<300> refreshCounter = hcn->getHttpRequestCookie ("refreshCounter");
    if (refreshCounter == "") 
      refreshCounter = "0";
    refreshCounter = atoi (refreshCounter) + 1;


    // 2️⃣ Set the cookie that will be sent to the browser through HTTP reply
    hcn->setHttpReplyCookie ("refreshCounter", refreshCounter, time (NULL) + 60); // cookie is valid 1 minute
                                                                                  // if time is set to 0 the cookie doesn't expire


    // 3️⃣ Return HTML content that will be sent to the browser through HTTP reply
    String httpReply = "<!DOCTYPE html>\n"
                       "<html lang='en'>\n"
                       "<head>\n"
                       "   <meta charset='UTF-8'>\n"
                       "   <title>Cookie example</title>\n"
                       "</head>\n"
                       "<body>\n"
                       "   <h1>Cookie example</h1>\n"
                       "   <p>This page has been refreshed ";
           httpReply += refreshCounter;
           httpReply += " times. Click refresh to see more.</p>\n"
                        "</body>\n"
                        "</html>";
    return httpReply;
  }

  // let the HTTP server hanle the request itself
  return "";
}

httpServer_t *httpServer = NULL;

void setup () {
  Serial.begin (115200);

  // Start WiFi connection
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


  // 4️⃣ Setting the time is necessary to set cookie expiration time
  // setenv ("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // or select another (POSIX) time zones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  ntpClient_t ntpClient ("1.si.pool.ntp.org", "2.si.pool.ntp.org", "3.si.pool.ntp.org");
  ntpClient.syncTime ();


  // ...
}

void loop () {

}
