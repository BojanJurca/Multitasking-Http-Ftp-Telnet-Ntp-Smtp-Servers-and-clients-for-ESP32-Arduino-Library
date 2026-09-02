/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
*/


#include <WiFi.h>


// 1️⃣ Include file system of your choice and then thread-safe wrapper arround it
#include <LittleFS.h>             // Or SPIFFS.h or FFat.h or SD.h ...
#include <threadSafeFS.h>         // Include thread-safe wrapper since SPIFFS, LittleFS, FFat and SD file systems are not thread safe


#include <ntpClient.h>            // Setting the time is not really necessary, only if you want to see the correct file creation times
#define HOSTNAME "Esp32Server"    // Choose your server's name - this is how FTP server would introduce itself to the clients
#include <ftpServer.h>


// 2️⃣ Use thread-safe wrapper for all file operations from now on in your code

void setup () {
  Serial.begin (115200);


  // 3️⃣ Start file system under (thread-safe wrapper) tsfs (LittleFS or SPIFFS or FFat or SD)
  tsfs.begin (true);


  // 4️⃣ Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


  // 5️⃣ Create static (so it would contiune to run even when setup finishes) FTP server instance that would use thread-safe wrapper tsfs arround LittleFS (or FFat or SD)
                                // Optional arguments:
  static ftpServer_t ftpServer; // threadSafeFS::FS& fileSystem 
                                // Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                // int serverPort = 21
                                // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                // bool runListenerInItsOwnTask = true


  // 6️⃣ Check if FTP server instance is created && FTP server is running
  if (ftpServer)
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
  setenv ("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // or select some other (POSIX) time zone: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  ntpClient_t ntpClient ("1.si.pool.ntp.org", "2.si.pool.ntp.org", "3.si.pool.ntp.org");
  ntpClient.syncTime ();


  // 9️⃣ Use tsfs thread-safe wrapper as you would use LittleFS in your code, for example:
  File f = tsfs.open ("/test.txt", "w");
  if (f) {
    f.print ("This is a test file.");
    f.close ();
  }


  // ... your code here
}

void loop () {

}
