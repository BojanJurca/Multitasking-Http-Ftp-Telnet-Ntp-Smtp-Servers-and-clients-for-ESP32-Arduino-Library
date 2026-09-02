/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
*/


#include <WiFi.h>

#include <LittleFS.h>             // Or SPIFFS.h or FFat.h or SD.h ...
#include <threadSafeFS.h>         // Include thread-safe wrapper since SPIFFS, LittleFS, FFat and SD file systems are not thread safe

#define HOSTNAME "Esp32Server"    // Choose your server's name - this is how FTP server would introduce itself to the clients
#include <ftpServer.h>


void setup () {
  Serial.begin (115200);

  // Start file system under (thread-safe wrapper) tsfs (LittleFS or SPIFFS or FFat or SD)
  tsfs.begin (true);
  
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // ... your code here
}

void loop () {

  // 1️⃣ Create static (so it would contiune to run even when loop finishes) FTP server instance
                                                              // Optional arguments:
  static ftpServer_t ftpServer (tsfs, NULL, 21, NULL, false); // threadSafeFS::FS& fileSystem 
																			                        // Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
																			                        // int serverPort = 21
  																	                          // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
																			                        // SET THIS ARGUMENT TO FALSE: bool runListenerInItsOwnTask = true
  // this would spare roughly 3 KB of memory that listener task would use otherwise

  /* Check if FTP server is running
  if (ftpServer && *ftpServer)
    Serial.println ("FTP server started");
  else
    Serial.println ("FTP server did not start");
  */


  // 2️⃣ Periodically call accept yourself since the listener is not running (in its own task)
  ftpServer.accept ();


  // ... your code here - should not block for too long - this would also block calling accept ()
}
