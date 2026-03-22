#include <WiFi.h>
#include <SPIFFS.h>               // Or LittleFS.h or FFat.h or SD.h ...
                                  //    SPIFFS seems to be the most stable choice although it has its limitations:
                                  //    speed, does not really support directories, does not support file times, shorter file names
#include <threadSafeFS.h>         // Include thread-safe wrapper since LittleFS, FFat and SD file systems are not thread safe


// 1️⃣ Crete thread-safe wrapper arround LittleFS (or FFat or SD)
threadSafeFS::FS TSFS (SPIFFS);   // Or LittleFS or FFat or SD ...


// 2️⃣ Use thread-safe wrapper for all file operations form now on in your code
using File = threadSafeFS::File;  


#include <httpServer.h>

httpServer_t *httpServer = NULL;


void setup () {
  Serial.begin (115200);


  // 3️⃣ Start LittleFS (or FFat or SD)
  SPIFFS.begin (true);


  // 4️⃣ Create some HTTP server files - or get them there with FTP for example
  TSFS.mkdir ("/var");
  TSFS.mkdir ("/var/www");
  TSFS.mkdir ("/var/www/html");

  File f;
  if ((f = TSFS.open ("/var/www/html/index.html", "w"))) {
    f.print ( "<!DOCTYPE html>\n"
              "<html lang='en'>\n"
              "<head>\n"
              "   <meta charset='UTF-8'>\n"
              "   <title>Example</title>\n"
              "   <link rel='stylesheet' href='/css/style.css'>\n"
              "</head>\n"
              "<body>\n"
              "   <h1>Example</h1>\n"
              "   <p id='message'>Hello world!</p>\n"
              "   <button id='btn'>Change the message</button>\n"
              "   <script src='/js/app.js'></script>\n"
              "</body>\n"
              "</html>" );

    f.close ();
  }

  TSFS.mkdir ("/var/www/html/css");

  if ((f = TSFS.open ("/var/www/html/css/style.css", "w"))) {
    f.print ( "body { font-family: sans-serif; background: #f5f5f5; margin: 2rem; }\n"
              "h1 { color: #333; }\n"
              "#message { padding: 0.5rem 1rem; background: #ffffff; border: 1px solid #ccc; display: inline-block; margin-right: 1rem; }\n"
              "#btn { padding: 0.5rem 1rem; border: none; background: #007acc; color: #fff; cursor: pointer; }\n"
              "#btn:hover { background: #005f99; }\n" );

    f.close ();
  }

  TSFS.mkdir ("/var/www/html/js");

  if ((f = TSFS.open ("/var/www/html/js/app.js", "w"))) {
    f.print ( "document.addEventListener('DOMContentLoaded', () => {\n"
              "   const msg = document.getElementById('message');\n"
              "   const btn = document.getElementById('btn');\n"
              "\n"
              "   btn.addEventListener('click', () => {\n"
              "      msg.textContent = 'Message changed from JavaScript.';\n"
              "   });\n"
              "});" );

    f.close ();
  }


  // Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");

  
  // 5️⃣ Create HTTP server instance passing it callback function that will handle the HTTP requests 
                                                        // optional arguments:
  httpServer = new (std::nothrow) httpServer_t (TSFS);  // threadSafeFS::FS& fileSystem,
                                                        // String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) = NULL,
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
