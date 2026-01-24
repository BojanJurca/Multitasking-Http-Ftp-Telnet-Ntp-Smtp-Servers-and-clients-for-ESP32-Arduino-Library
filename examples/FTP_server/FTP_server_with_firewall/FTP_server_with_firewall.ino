#include <WiFi.h>
#include <LittleFS.h>
#include <threadSafeFS.h>         // Include thread-safe wrapper since LittleFS, FFat and SD file systems are not thread safe
threadSafeFS::FS TSFS (LittleFS); // Crete thread-safe wrapper arround LittleFS (or FFat or SD)
using File = threadSafeFS::File;  // Use thread-safe wrapper for all file operations form now on in your code
#define HOSTNAME "Esp32Server"    // Choose your server's name - this is how FTP server would introduce itself to the clients
#include <ftpServer.h>


ftpServer_t *ftpServer = NULL;

// 1️⃣ provide a firewall callback function to FTP server that would tell which connctions to accept and which to refuse
bool firewallCallback (char *clientIP, char *serverIP) { 

  // Must be reentrant !!

  // accept only connections from local network, for example 10.18.1.*
  if (strstr (clientIP, "10.18.1.") == clientIP)
    return true;
  else 
    return false; 
}


void setup () {
  Serial.begin (115200);
  LittleFS.begin (true);
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 2️⃣ create FTP server instance that would use LittleFS with firewall callback function
  ftpServer = new ftpServer_t (TSFS, NULL, 21, firewallCallback); // optional arguments:
                                                                  //    Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                                                  //    int serverPort = 21
                                                                  //    bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                                                  //    bool runListenerInItsOwnTask = true

  // check if FTP server instance is created && FTP server is running
  if (ftpServer && *ftpServer)
    Serial.println ("FTP server started");
  else
    Serial.println ("FTP server did not start");


  // ...
}

void loop () {

}
