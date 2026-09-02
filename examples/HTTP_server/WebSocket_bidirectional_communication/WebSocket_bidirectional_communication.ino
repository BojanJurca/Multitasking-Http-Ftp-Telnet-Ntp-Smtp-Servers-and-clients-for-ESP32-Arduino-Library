/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino/blob/master/PROJECT_STATE.md
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
            "      <title>Websocket Bidirectional Communication</title>\n"
            "   </head>\n"
            "   <body>\n"
            "      <h1>Websocket Bidirectional Communication</h1>\n"
            "		   <br><br>\n"
            "\n"
            "      <label for='inputField'>Enter text or comma separated integers:</label><br>\n"
            "      <input type='text' id='inputField' style='width: 400px;'><br><br>\n"
            "\n"
            "      <button onclick='sendText ()'>Send text to server</button>\n"
            "      <button onclick='sendBinary ()'>Send binary data to server</button>\n"
            "\n"
            "   </body>\n"            
            "   <script type='text/javascript'>\n"
            "\n"
            "      if ('WebSocket' in window) {\n"
            "         var ws = new WebSocket ('ws://' + self.location.host + '/bidirectionalCommunication'); // open WebSocket connection\n"
            "\n"
            "         function sendText () {\n"
            "            var text = document.getElementById ('inputField').value;\n"
            "\n"     
            "            ws.send (text); // send text (browser -> server)\n"
            "\n"
            "         };\n"
            "\n"
            "        function sendBinary () {\n"
            "            const raw = document.getElementById ('inputField').value.trim ();\n"
            "            const parts = raw.split (',').map (s => s.trim ()); // split on ,\n"
            "            // check if all parts are numbers\n"
            "            if (!parts.every (p => /^-?\\d+$/.test (p))) {\n"
            "                alert ('Input is not a list of integers.');\n"
            "                return;\n"
            "            }\n"
            "            // convert to Int16Array\n"
            "            const nums = parts.map (n => parseInt (n, 10));\n"
            "            const int16 = new Int16Array (nums);\n"
            "\n"
            "            ws.send (int16.buffer); // send binary data (browser -> server)\n"
            "\n"
            "        };\n"
            "\n"
            "      		ws.onopen = function () {;};\n"
            "\n"
            "      		ws.onmessage = function (evt) {\n" 
            "\n"
            "           if (typeof (evt.data) === 'string' || evt.data instanceof String) { // UTF-8 formatted string data\n"
            "             alert ('Got text from server:\\n' + evt.data); // (server -> browser)\n"
	          "           }\n"
            "\n"
            "        		if (evt.data instanceof Blob) {\n"
            "        		   // receive binary data as blob and then convert it into array of some type, say Int16\n"
            "        			 var myByteArray = null;\n"
            "      	  		 var myArrayBuffer = null;\n"
            "      		  	 var myFileReader = new FileReader ();\n"
            "      			   myFileReader.onload = function (event) {\n"
            "      			      myArrayBuffer = event.target.result;\n"
            "                 const myInt16Array = new Int16Array (myArrayBuffer); // treat binary data as 16 bit integers\n"
            "                 alert ('Got 16 bit integers from browser:\\n' + myInt16Array.join (', ')); // (server -> browser)\n"
            "        			 };\n"
            "      	  		 myFileReader.readAsArrayBuffer (evt.data);\n"
            "      	    }\n"
            "         };\n"
            "\n"
            "         ws.onclose = function () {\n" 
            "           alert ('WebSocket connection is closed.');\n"
            "         };\n"
            "\n"
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

  if (httpRequestIs ("GET /bidirectionalCommunication ")) { // 2️⃣ Handle the WebSocket request

    while (true) {
        switch (webSck->peek ()) {
            case 0:                   // data not ready (yet)
                                      delay (10);
                                      break;
            case STRING_FRAME_TYPE:   { // 1 = text received
                                        char s [300];
                                        // browser -> server
                                        webSck->recvString (s, sizeof (s) - 1); // if the buffer is full the closing 0 will be placed at s [sizeof (s) - 1]
                                        
                                        Serial.printf ("Got text from browser: '%s'. Sending back the confirmation.\n", s);

                                        // server -> browser
                                        if (!webSck->sendString ("I've got your text, thak you."))
                                          return; // error, close this connection
                                        break;
                                      }
            case BINARY_FRAME_TYPE:   { // 2 = binary data received
                                        byte buffer [256];
                                        // browser -> server
                                        int bytesRead = webSck->recvBlock (buffer, sizeof (buffer));
                                        int integersRead = bytesRead / sizeof (int16_t);

                                        Serial.printf ("Got %i bytes from browser. Suppose they represent %i 16-bit integers. Sending back running sum.\n", bytesRead, integersRead);

                                        // server -> browser
                                        if (!webSck->sendString ("I've got your data, thak you. I'm sending back running sum."))
                                          return; // error, close this connection

                                        // calculate running sum, just for the purpose of this example
                                        int16_t *samples = (int16_t *) buffer;
                                        int16_t runningSum = 0;
                                        for (int i = 0; i < integersRead; i++) {
                                          runningSum += samples [i];
                                          samples [i] = runningSum;
                                        }

                                        // server -> browser
                                        if (!webSck->sendBlock ((byte *) samples, integersRead * sizeof (int16_t)))
                                          return; // error, close this connection
                                        break;
                                      }
            default:                  // -1 = error, cosed or time-out
                                      Serial.printf ("Error, connection closed by browser or time-out.\n");
                                      return; // close this connection
        }
    }

  }

}


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
