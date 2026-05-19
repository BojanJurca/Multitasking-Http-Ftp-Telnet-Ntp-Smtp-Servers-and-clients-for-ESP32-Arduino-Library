/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino/blob/master/PROJECT_STATE.md


  The HTTPS server uses the WolfSSL library, which can be installed through the 
  Arduino IDE or downloaded from GitHub: https://github.com/wolfSSL/wolfssl.

  WolfSSL must be installed and configured to match your ESP32 board and the type
  of TLS certificate you plan to use before compiling this example.

  A sample user_settings.h file is included to demonstrate a recommended configuration 
  for ESP32-WROOM modules using ECC certificates.

  Additionally, example OpenSSL commands are provided to generate a self‑signed ECC 
  certificate and private key suitable for use with ESP32 

*/


#include <WiFi.h>
#include <LittleFS.h>             // Or SPIFFS.h or FFat.h or SD.h ...
#include <threadSafeFS.h>         // Include thread-safe wrapper since LittleFS, FFat and SD file systems are not thread safe


// 1️⃣ Use file system HTTPS server for it will be needed anyway to store web session tokens
threadSafeFS::FS TSFS (LittleFS);   // Or SPIFFS or FFat or SD ...
using File = threadSafeFS::File;  


// 2️⃣ Use NTP client to get current time - it will be needed to set web session token expiration time
#include <ntpClient.h>
ntpClient_t ntpClient ("1.si.pool.ntp.org", "2.si.pool.ntp.org", "3.si.pool.ntp.org");


#include <./https/httpsServer.h>
httpsServer_t *httpsServer = NULL;

#include "webSessionTokens.h"
webSessionTokens_t *webSessionTokens = NULL;


// 3️⃣ Manage HTTP requests
String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) {

    // Must be reentrant !!!


    #define httpRequestIs(X) (strstr(httpRequest,X)==httpRequest)

    // ----- unrestricted part -----

    // ...

    // ----- entering restricted part - check authorization -----

    Cstring<300> token = hcn->getHttpRequestCookie ("session");
    Cstring<64> userName;
    const char *cipherName = hcn->cipherName ();

    if (token != "" && webSessionTokens)
        userName = webSessionTokens->getUserNameFromToken (token);

    // login-logout REST API
    if (httpRequestIs ("POST /login/"))                 {   // POST login/username/password ...
                                                            if (*cipherName == 0)
                                                                return "Login is alowed through secure HTTPS protocol only";
                                                            // parse URL to get userName and password
                                                            Cstring<64>userName;
                                                            Cstring<64>password;
                                                            if (sscanf (httpRequest + 12, "%64[^/]/%64s", (char *) &userName, (char *) &password) != 2)
                                                                return "Missing username or password";

                                                            // TO DO: implement your user management

                                                            if (userName != "user" && password != "password")
                                                                return "Wrong username or password";
                                                            // create new session token
                                                            token = "";
                                                            if (webSessionTokens && time (NULL) > 1600000000) // 1600000000 ~2020
                                                                token = webSessionTokens->newToken (userName, time (NULL) + 86400); // 86400 = 1 day
                                                            if (token == "")
                                                                return "Couldn't create session token";
                                                            // clean up token store every now and then
                                                            webSessionTokens->deleteExpiredTokens ();
                                                            // pass the token to session cookie
                                                            hcn->setHttpReplyCookie ("session", token, time (NULL) + 86400);
                                                            return "OK";
                                                        } 
    else if (httpRequestIs ("POST /logout "))           {
                                                            // delete session cookie
                                                            hcn->setHttpReplyCookie ("session", "", 1); // 0 means that the cookie never expires, 1 will always expire 
                                                            if (webSessionTokens) {            
                                                                // delete session token
                                                                webSessionTokens->deleteToken (token);
                                                                // clean up token store every now and then
                                                                webSessionTokens->deleteExpiredTokens ();
                                                            }
                                                            return "OK";
                                                        }

    // ----- protect some pages if userName != "" the token is valid, proceed, if not redirect to login.html -----
    if (userName == "") {
      
        if (httpRequestIs ("GET /administration.html")) {   // do not include space after ...html since users may also pass parameters: ...html?...
                                                            hcn->setHttpReplyHeaderField ("Location", "/login.html?redirect=/administration.html");
                                                            hcn->setHttpReplyStatus ("307 Login required"); // 307 = redirect
                                                            return "Login required"; // whatever
                                                        }

    } else {

        // ----- restrictes part for logged-in users only -----

        // you may also want to check userName rights at this point

        // TO DO: put your restricted access code here 

    }

    // ----- unrestricted part again -----

    return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning ""
}

void setup () {
  Serial.begin (115200);
  LittleFS.begin (true);


  // 4️⃣ Create some .html files - or get them there with FTP for example
  TSFS.mkdir ("/var");
  TSFS.mkdir ("/var/www");
  TSFS.mkdir ("/var/www/html");

  File f;

  // if (!TSFS.isFile ("/var/www/html/index.html")) 
  {
    #include "index.h"
    if ((f = TSFS.open ("/var/www/html/index.html", "w"))) {
      f.print (index);
      f.close ();
    }
  }

  // if (!TSFS.isFile ("/var/www/html/login.html")) 
  {
    #include "login.h"
    if ((f = TSFS.open ("/var/www/html/login.html", "w"))) {
      f.print (login);
      f.close ();
    }
  }

  // if (!TSFS.isFile ("/var/www/html/administration.html")) 
  {
    #include "administration.h"
    if ((f = TSFS.open ("/var/www/html/administration.html", "w"))) {
      f.print (administration);
      f.close ();
    }
  }


  // Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 5️⃣ Create HTTPS server instance passing it callback function that will handle the HTTP requests 
                                                                                // optional arguments:
  httpsServer = new (std::nothrow) httpsServer_t (TSFS,                         // threadSafeFS::FS& fileSystem,
                                                  httpRequestHandlerCallback);  // String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) = NULL,
                                                                                // void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                                                                // int serverPort = 443,
                                                                                // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                                                                // bool runListenerInItsOwnTask = true

  // Check if HTTPS server instance is created && HTTPS server is running
  if (httpsServer && *httpsServer)
    Serial.println ("HTTPS server started");
  else
    Serial.println ("HTTPS server did not start");


  webSessionTokens = new (std::nothrow) webSessionTokens_t (TSFS);
  if (!webSessionTokens)
    Serial.println ("[webSessionTokens] not created");


  // Use web browser to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000); 
      Serial.println ("   ."); 
  } 
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());

  ntpClient.syncTime ();


  // ...
}

void loop () {

}
