#include <WiFi.h>
#include <LittleFS.h>             // Or FFat.h or/and SD.h
#include <threadSafeFS.h>         // Include thread-safe wrapper since LittleFS, FFat and SD file systems are not thread safe
using File = threadSafeFS::File;  // Use thread-safe wrapper for all file operations form now on in your code
#include <ntpClient.h>            // Setting the time is not really necessary, only if you want to see the correct file creation times
#define HOSTNAME "Esp32Server"    // Choose your server's name - this is how FTP server would introduce itself to the clients
#include <ftpServer.h>


// 1️⃣ Crete thread-safe wrapper arround LittleFS (or FFat or SD)
threadSafeFS::FS TSFS (LittleFS);

// 2️⃣ Use thread-safe wrapper for all file operations form now on in your code
using File = threadSafeFS::File;

ftpServer_t *ftpServer = NULL;

void setup () {
  Serial.begin (115200);

  // 3️⃣ Start LittleFS (or FFat or SD)
  LittleFS.begin (true);

  // 4️⃣ Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 5️⃣ Create FTP server instance that would use thread-safe wrapper arround LittleFS (or FFat or SD)
  ftpServer = new (std::nothrow) ftpServer_t (TSFS);  // optional arguments:
                                                      //    Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                                      //    int serverPort = 21
                                                      //    bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                                      //    bool runListenerInItsOwnTask = true

  // 6️⃣ Check if FTP server instance is created && FTP server is running
  if (ftpServer && *ftpServer)
    Serial.println ("FTP server started");
  else
    Serial.println ("FTP server did not start");

  // 7️⃣ Use FTP client to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000); 
      Serial.println ("   ."); 
  } 
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());

  // 8️⃣ Setting the time is not really necessary, only if you want to see the correct file creation times
  setenv ("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // or select another (POSIX) time zones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  ntpClient_t ntpClient ("1.si.pool.ntp.org", "2.si.pool.ntp.org", "3.si.pool.ntp.org");
  ntpClient.syncTime ();

  // 9️⃣ Use thread-safe wrapper as you would use LittleFS in your code, for example:
  File f = TSFS.open ("/test.txt", "w");
  if (f) {
    f.print ("This is a test file.");
    f.close ();
  }

}

void loop () {

}
