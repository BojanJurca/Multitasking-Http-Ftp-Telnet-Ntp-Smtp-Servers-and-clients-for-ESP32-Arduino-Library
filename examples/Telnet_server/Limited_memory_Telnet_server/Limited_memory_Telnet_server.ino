/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
*/


#include <WiFi.h>
#include <ntpClient.h>            // NTP client is needed only for time related commands
#define HOSTNAME "Esp32Server"    // Choose your server's name - this is how Telnet server would introduce itself to the clients

// Choose which built-in Telnet commands will be included prior to #include <telnetServer.h>
#define TELNET_CLEAR_COMMAND 1        // 0=exclude, 1=include, clear is included by default
#define TELNET_UNAME_COMMAND 1        // 0=exclude, 1=include, uname is included by default
#define TELNET_FREE_COMMAND 0         // 0=exclude, 1=include, free is included by default
#define TELNET_NOHUP_COMMAND 0        // 0=exclude, 1=include, nohup is included by default
#define TELNET_REBOOT_COMMAND 1       // 0=exclude, 1=include, reboot is included by default
#define TELNET_DMESG_COMMAND 1        // 0=exclude, 1=include, dmesg is included by default
#define TELNET_QUIT_COMMAND 1         // 0=exclude, 1=include, quit is included by default
#define TELNET_UPTIME_COMMAND 1       // 0=exclude, 1=include, date is included by default
#define TELNET_DATE_COMMAND 1         // 0=exclude, 1=include, date is included by default
#define TELNET_NTPDATE_COMMAND 1      // 0=exclude, 1=include, ntpdate is included by default
#define TELNET_CRONTAB_COMMAND 0      // 0=exclude, 1=include, crontab is included by default
#define TELNET_PING_COMMAND 1         // 0=exclude, 1=include, ping is included by default
#define TELNET_IFCONFIG_COMMAND 1     // 0=exclude, 1=include, ifconfig is included by default
#define TELNET_IW_COMMAND 0           // 0=exclude, 1=include, iw is included by default
#define TELNET_NETSTAT_COMMAND 0      // 0=exclude, 1=include, netstat is included by default
#define TELNET_KILL_COMMAND 0         // 0=exclude, 1=include, kill is included by default
#define TELNET_CURL_COMMAND 0         // 0=exclude, (default) 1=include http, 2=include http and https
#define TELNET_SENDMAIL_COMMAND 0     // 0=exclude, 1=include, sendmail is included by default
#define TELNET_LS_COMMAND 0           // 0=exclude, 1=include, ls is included by default
#define TELNET_TREE_COMMAND 0         // 0=exclude, 1=include, tree is included by default
#define TELNET_FSINFO_COMMAND 0       // 0=exclude, 1=include, fsinfo is included by default
#define TELNET_MKDIR_COMMAND 0        // 0=exclude, 1=include, mkdir is included by default
#define TELNET_RMDIR_COMMAND 0        // 0=exclude, 1=include, rmdir is included by default
#define TELNET_CD_COMMAND 0           // 0=exclude, 1=include, cd is included by default
#define TELNET_PWD_COMMAND 0          // 0=exclude, 1=include, pwd is included by default
#define TELNET_CAT_COMMAND 0          // 0=exclude, 1=include, cat is included by default
#define TELNET_VI_COMMAND 0           // 0=exclude, 1=include, vi is included by default
#define TELNET_CP_COMMAND 0           // 0=exclude, 1=include, cp is included by default
#define TELNET_RM_COMMAND 0           // 0=exclude, 1=include, rm is included by default
#define TELNET_LSOF_COMMAND 0         // 0=exclude, 1=include, lsof is included by default
#define TELNET_DIGITAL_READ_COMMAND 1 // 0=exclude, 1=include, digitalRead is included by default

#define SWAP_DEL_AND_BACKSPACE 0      // set to 1 to swap the meaning of these keys - this would be suitable for Putty and Linux Telnet clients

#include <telnetServer.h>


void setup () {
  Serial.begin (115200);
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");

  // Use Telent client to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000); 
      Serial.println ("   ."); 
  } 
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());

  // Setting the time is only important for time related commands
  setenv ("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // or select some other (POSIX) time zone: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  ntpClient_t ntpClient ("1.si.pool.ntp.org", "2.si.pool.ntp.org", "3.si.pool.ntp.org");
  ntpClient.syncTime ();


  // ... your code here
}

void loop () {


  // 1️⃣ Create static (so it would contiune to run even when loop finishes) Telnet server instance
                                                                    // Optional arguments (when thread-safe file system is not included):
  static telnetServer_t telnetServer (NULL, NULL, 23, NULL, false); // Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                                                    // String (*telnetCommandHandlerCallback) (int argc, char *argv [], telnetConnection_t *tcn) = NULL
                                                                    // int serverPort = 23
                                                                    // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                                                    // SET THIS ARGUMENT TO FALSE: bool runListenerInItsOwnTask = true
  // this would spare roughly 3 KB of memory that listener task would use otherwise

  /* Check if Telnet server is running
  if (telnetServer)
    Serial.println ("Telnet server started");
  else
    Serial.println ("Telnet server did not start");
  */


  // 2️⃣ Periodically call accept yourself since the listener is not running (in its own task)
  telnetServer.accept ();


    // ... your code here - should not block for too long - this would also block calling accept ()
}
