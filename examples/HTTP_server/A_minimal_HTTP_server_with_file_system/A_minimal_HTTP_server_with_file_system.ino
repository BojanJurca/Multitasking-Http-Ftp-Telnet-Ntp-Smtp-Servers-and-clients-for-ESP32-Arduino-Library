/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
*/


#include <WiFi.h>


// 1️⃣ Include file system of your choice and then thread-safe wrapper arround it
#include <SPIFFS.h>               // Or LittleFS.h or FFat.h or SD.h
#include <threadSafeFS.h>         // Include thread-safe wrapper since SPIFFS, LittleFS, FFat and SD file systems are not thread safe


#include <httpServer.h>


void setup () {
  Serial.begin (115200);


  // 2️⃣ Start file system under (thread-safe wrapper) tsfs (LittleFS or SPIFFS or FFat or SD)
  tsfs.begin (true);


  // 3️⃣ Create some HTTP server files
  // In a real project we could include FTP server as well and just FTP the files from a computer to /var/www/html directory
  tsfs.mkdir ("/var");
  tsfs.mkdir ("/var/www");
  tsfs.mkdir ("/var/www/html");

  File f;
  if ((f = tsfs.open ("/var/www/html/index.html", "w"))) {
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

  tsfs.mkdir ("/var/www/html/css");

  if ((f = tsfs.open ("/var/www/html/css/style.css", "w"))) {
    f.print ( "body { font-family: sans-serif; background: #f5f5f5; margin: 2rem; }\n"
              "h1 { color: #333; }\n"
              "#message { padding: 0.5rem 1rem; background: #ffffff; border: 1px solid #ccc; display: inline-block; margin-right: 1rem; }\n"
              "#btn { padding: 0.5rem 1rem; border: none; background: #007acc; color: #fff; cursor: pointer; }\n"
              "#btn:hover { background: #005f99; }\n" );

    f.close ();
  }

  tsfs.mkdir ("/var/www/html/js");

  if ((f = tsfs.open ("/var/www/html/js/app.js", "w"))) {
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


  // 4️⃣ Create static (so it would contiune to run even when setup finishes) HTTP server instance that would use thread-safe wrapper tsfs arround SPIFFS or LittleFS or FFat or SD
                                  // Optional arguments (when file system is included):
  static httpServer_t httpServer;// (tsfs); // threadSafeFS::FS& fileSystem,
                                  // String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) = NULL,
                                  // void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                  // int serverPort = 80,
                                  // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                  // bool runListenerInItsOwnTask = true


  // 5️⃣ Check if HTTP server is running
  if (httpServer)
    Serial.println ("HTTP server started");
  else
    Serial.println ("HTTP server did not start");


  // Use web browser to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000); 
      Serial.println ("   .");
  }
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());


  // ... your code here
}

void loop () {

}
