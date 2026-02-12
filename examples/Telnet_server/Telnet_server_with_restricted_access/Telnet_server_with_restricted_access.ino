#include <WiFi.h>
#include <LittleFS.h>             // Or FFat.h or/and SD.h
#include <threadSafeFS.h>         // Include thread-safe wrapper since LittleFS, FFat and SD file systems are not thread safe
using File = threadSafeFS::File;  // Use thread-safe wrapper for all file operations form now on in your code
#include <ntpClient.h>            // NTP client is neede only for time commands
#define HOSTNAME "Esp32Server"    // Choose your server's name - this is how Telnet server would introduce itself to the clients


// Choose which built-in Telnet commands will be included
#define TELNET_CLEAR_COMMAND 0      // 0=exclude, 1=include, clear is included by default
#define TELNET_UNAME_COMMAND 0      // 0=exclude, 1=include, uname is included by default
#define TELNET_FREE_COMMAND 0       // 0=exclude, 1=include, free is included by default
#define TELNET_NOHUP_COMMAND 0      // 0=exclude, 1=include, nohup is included by default
#define TELNET_REBOOT_COMMAND 1     // 0=exclude, 1=include, reboot is included by default
#define TELNET_DMESG_COMMAND 1      // 0=exclude, 1=include, dmesg is included by default
#define TELNET_QUIT_COMMAND 1       // 0=exclude, 1=include, quit is included by default
#define TELNET_UPTIME_COMMAND 1     // 0=exclude, 1=include, date is included by default
#define TELNET_DATE_COMMAND 1       // 0=exclude, 1=include, date is included by default
#define TELNET_NTPDATE_COMMAND 1    // 0=exclude, 1=include, ntpdate is included by default
#define TELNET_PING_COMMAND 1       // 0=exclude, 1=include, ping is included by default
#define TELNET_IFCONFIG_COMMAND 1   // 0=exclude, 1=include, ifconfig is included by default
#define TELNET_IW_COMMAND 0         // 0=exclude, 1=include, iw is included by default
#define TELNET_NETSTAT_COMMAND 0    // 0=exclude, 1=include, netstat is included by default
#define TELNET_KILL_COMMAND 0       // 0=exclude, 1=include, kill is included by default
#define TELNET_CURL_COMMAND 0       // 0=exclude, 1=include, curl is included by default
#define TELNET_SENDMAIL_COMMAND 0   // 0=exclude, 1=include, sendmail is included by default
#define TELNET_LS_COMMAND 1         // 0=exclude, 1=include, ls is included by default
#define TELNET_TREE_COMMAND 0       // 0=exclude, 1=include, tree is included by default
#define TELNET_MKDIR_COMMAND 1      // 0=exclude, 1=include, mkdir is included by default
#define TELNET_RMDIR_COMMAND 1      // 0=exclude, 1=include, rmdir is included by default
#define TELNET_CD_COMMAND 1         // 0=exclude, 1=include, cd is included by default
#define TELNET_PWD_COMMAND 1        // 0=exclude, 1=include, pwd is included by default
#define TELNET_CAT_COMMAND 0        // 0=exclude, 1=include, cat is included by default
#define TELNET_VI_COMMAND 0         // 0=exclude, 1=include, vi is included by default
#define TELNET_CP_COMMAND 0         // 0=exclude, 1=include, cp is included by default
#define TELNET_RM_COMMAND 0         // 0=exclude, 1=include, rm is included by default
#define TELNET_LSOF_COMMAND 0       // 0=exclude, 1=include, lsof is included by default

#define SWAP_DEL_AND_BACKSPACE 0    // seto to 1 to swap the meaning of these keys - this would be suitable for Putty and Linux Telnet clients

#include <telnetServer.h>

// Crete thread-safe wrapper arround LittleFS (or FFat or SD)
threadSafeFS::FS TSFS (LittleFS);

telnetServer_t *telnetServer = NULL;


// 1️⃣ provide a callback function to Telnet server that checks user access rights
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


  // 2️⃣ create Telent server instance that would use LittleFS and getUserHomeDirectory callback function to check user rights
  telnetServer = new (std::nothrow) telnetServer_t (TSFS, getUserHomeDirectoryCallback);  // optional arguments:
                                                                                          // Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                                                                          // String (*telnetCommandHandlerCallback) (int argc, char *argv [], telnetConnection_t *tcn) = NULL
                                                                                          // int serverPort = 23
                                                                                          // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                                                                          // bool runListenerInItsOwnTask = true
                                                        
                                                        
  // Check if Telnet server instance is created && Telnet server is running
  if (telnetServer && *telnetServer)
    Serial.println ("Telnet server started");
  else
    Serial.println ("Telnet server did not start");

  // Use Telent client to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000); 
      Serial.println ("   ."); 
  } 
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());


  // ...
}

void loop () {

}
