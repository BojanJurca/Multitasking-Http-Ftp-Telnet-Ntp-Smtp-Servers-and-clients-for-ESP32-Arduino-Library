#include <WiFi.h>
#include <LittleFS.h>
#include <threadSafeFS.h>         // Include thread-safe wrapper since LittleFS, FFat and SD file systems are not thread safe
threadSafeFS::FS TSFS (LittleFS); // Crete thread-safe wrapper arround LittleFS (or FFat or SD)
using File = threadSafeFS::File;  // Use thread-safe wrapper for all file operations form now on in your code
#define HOSTNAME "Esp32Server"    // Choose your server's name - this is how FTP server would introduce itself to the clients
#include <ftpServer.h>


ftpServer_t *ftpServer = NULL;

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
  LittleFS.begin (true);
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 2️⃣ create FTP server instance that would use LittleFS and getUserHomeDirectory callback function to check user rights
  ftpServer = new ftpServer_t (TSFS, getUserHomeDirectoryCallback); // optional arguments:
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
