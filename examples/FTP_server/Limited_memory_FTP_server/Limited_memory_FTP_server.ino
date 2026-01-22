#include <WiFi.h>
#include <LittleFS.h>
#include <threadSafeFS.h>         // Include thread-safe wrapper since LittleFS, FFat and SD file systems are not thread safe
threadSafeFS::FS TSFS (LittleFS); // Crete thread-safe wrapper arround LittleFS (or FFat or SD)
using File = threadSafeFS::File;  // Use thread-safe wrapper for all file operations form now on in your code
#define HOSTNAME "Esp32Server"    // Choose your server's name - this is how FTP server would introduce itself to the clients
#include <ftpServer.h>


ftpServer_t *ftpServer = NULL;


void setup () {
  Serial.begin (115200);
  LittleFS.begin (true);
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 1️⃣ Create FTP server instance without listener's task (runListenerInItsOwnTask = false)
  // FTP server's listener would have to host in the loop task in this case, 
  // not having a seperate listening task would save 3 KB of memory
  ftpServer = new ftpServer_t (TSFS, NULL, 21, NULL, false);  // optional arguments:
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

  // 2️⃣  Periodically call accept
  if (ftpServer)
    ftpServer->accept ();


    // ... add your own code - it shoud not block for too long
}
