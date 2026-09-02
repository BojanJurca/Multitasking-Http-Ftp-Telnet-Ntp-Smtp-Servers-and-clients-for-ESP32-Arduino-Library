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

ftpServer_t *ftpServer = NULL;


// 1️⃣ provide a firewall callback function to FTP server that would tell which connctions to accept and which to refuse
bool firewallCallback (char *clientIP, char *serverIP) { 

  // Must be reentrant !!


  // accept only connections from local network, for example:
  if (strstr (clientIP, "192.168.1.") == clientIP)
    return true;
  else 
    return false; 
}

void setup () {
  Serial.begin (115200);

  // Start file system under (thread-safe wrapper) tsfs (LittleFS or SPIFFS or FFat or SD)
  tsfs.begin (true);
  
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 2️⃣ create FTP server instance that would use LittleFS with firewall callback function
                                                                                        // Optional arguments:
  ftpServer = new (std::nothrow) ftpServer_t (/* tsfs, */ NULL, 21, firewallCallback);  // threadSafeFS::FS& fileSystem 
                                                                                        // Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                                                                        // int serverPort = 21
                                                                                        // SET THIS ARGUMENT: bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                                                                        // bool runListenerInItsOwnTask = true

  // check if FTP server instance is created && FTP server is running
  if (ftpServer && *ftpServer)
    Serial.println ("FTP server started");
  else
    Serial.println ("FTP server did not start");


  // ... your code here
}

void loop () {

}
