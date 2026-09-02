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


// 1️⃣ provide a callback function to FTP server that checks user access rights
Cstring<255> getUserHomeDirectoryCallback (const Cstring<64>& userName, const Cstring<64>& password) {

    // Must be reentrant !!


    if (userName == "root" && password == "rootpassword")
        return "/";             // full access to the file system          
    if (userName == "bojan" && password == "bojanpassword")
        return "/home/bojan";   // limited access to the file system
    return "";                  // access (login) denyed
}


void setup () {
  Serial.begin (115200);

  // Start file system under (thread-safe wrapper) tsfs (LittleFS or SPIFFS or FFat or SD)
  tsfs.begin (true);
  
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 2️⃣ Create static (so it would contiune to run even when setup finishes) FTP server instance that would use thread-safe wrapper tsfs arround LittleFS (or FFat or SD)
                                                                // Optional arguments:
  static ftpServer_t ftpServer (getUserHomeDirectoryCallback);  // threadSafeFS::FS& fileSystem 
                                                                // SET THIS ARGUMENT: Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                                                // int serverPort = 21
                                                                // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                                                // bool runListenerInItsOwnTask = true

  // Check if FTP server is running
  if (ftpServer)
    Serial.println ("FTP server started");
  else
    Serial.println ("FTP server did not start");


  // ... your code here
}

void loop () {

}
