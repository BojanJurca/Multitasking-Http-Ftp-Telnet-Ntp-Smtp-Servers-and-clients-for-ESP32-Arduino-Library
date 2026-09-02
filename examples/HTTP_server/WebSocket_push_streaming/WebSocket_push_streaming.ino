/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
*/


#include <WiFi.h>
#include <httpServer.h>


// 1️⃣ A callback function that would provide HTML page to the browser. HTML file would open a WebSocket then.
//    You can also just FTP index.html file to /var/www/html instead 
String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) {

  // Must be reentrant !!!


  #define httpRequestIs(X) (strstr(httpRequest,X)==httpRequest)

  if (httpRequestIs ("GET / ") || httpRequestIs ("GET /index.html ")) {         // HTML page that will be sent to the browser
    return  "<!DOCTYPE html>\n"
            "<html lang='en'>\n"
            "   <head>\n"
            "      <meta charset='UTF-8'>\n"
            "      <title>WebSocket Push Streaming</title>\n"
            "   </head>\n"
            "   <body>\n"
            "      <h1>WebSocket Push Streaming</h1>\n"
            "		   <br><br>\n"
            "		   <span>RSSI: </span><span id='RSSI'> </span><span> dBm</span>\n"
            "   </body>\n"
            "   <script type='text/javascript'>\n"
            "\n"
            "      if ('WebSocket' in window) {\n"
            "         var ws = new WebSocket ('ws://' + self.location.host + '/rssiReader'); // open webSocket connection\n" // 2️⃣ This is the WebSocket request
            "      		ws.onopen = function () {;};\n"
            "      		ws.onmessage = function (evt) {\n" 
            "      		if (evt.data instanceof Blob) { // RSSI readings as 1 byte binary data\n"
            "      		   // receive binary data as blob and then convert it into array\n"
            "      			 var myByteArray = null;\n"
            "      			 var myArrayBuffer = null;\n"
            "      			 var myFileReader = new FileReader ();\n"
            "      			 myFileReader.onload = function (event) {\n"
            "      			    myArrayBuffer = event.target.result;\n"
            "      					myByteArray = new Int8Array (myArrayBuffer); // RSSI information is now in the array of size 1\n"
            "      					document.getElementById ('RSSI').innerText = myByteArray [0].toString ();\n"
            "      			 };\n"
            "      			 myFileReader.readAsArrayBuffer (evt.data);\n"
            "      	  }\n"
            "      }\n"
            "   } else {\n"
            "      alert ('WebSockets are not supported by your browser.');\n"
            "   }\n"
            "\n"
            "   </script>\n"
            "</html>";
  }

  // Let the HTTP server handle the request itself
  return "";
}


// 3️⃣ A callback function that would handle HTTP WebSocket requests
void wsRequestHandlerCallback (const char *httpRequest, httpServer_t::webSocket_t *webSck) {

  // Must be reentrant !!!


  #define httpRequestIs(X) (strstr(httpRequest,X)==httpRequest)

  if (httpRequestIs ("GET /rssiReader ")) { // 2️⃣ Handle the WebSocket request
    char c;
    do {
        delay (100);
        c = (char) WiFi.RSSI ();
    } while (webSck->sendBlock ((byte *) &c, sizeof (c))); // keep sending RSSI information as long as the browser is receiving it
  }
}


httpServer_t *httpServer = NULL;

void setup () {
  Serial.begin (115200);

  // Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 4️⃣ Create static (so it would contiune to run even when setup finishes) HTTP server instance
                                                              // Optional arguments (when file system is not included):  
  static httpServer_t httpServer (httpRequestHandlerCallback, // String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) = NULL,
                                  wsRequestHandlerCallback);  // SET THIS ARGUMENT: void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                                              // int serverPort = 80,
                                                              // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                                              // bool runListenerInItsOwnTask = true

  // Check if HTTP server instance is created && HTTP server is running
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
