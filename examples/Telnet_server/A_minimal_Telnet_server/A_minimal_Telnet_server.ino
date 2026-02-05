#include <WiFi.h>
#include <ntpClient.h>            // NTP client is neede only for time commands
#define HOSTNAME "Esp32Server"    // Choose your server's name - this is how Telnet server would introduce itself to the clients


// 1️⃣ Choose which built-in Telnet commands will be included
#define TELNET_CLEAR_COMMAND 1      // 0=exclude, 1=include, clear included by default
#define TELNET_UNAME_COMMAND 1      // 0=exclude, 1=include, uname included by default
#define TELNET_FREE_COMMAND 0       // 0=exclude, 1=include, free included by default
#define TELNET_NOHUP_COMMAND 0      // 0=exclude, 1=include, nohup included by default
#define TELNET_REBOOT_COMMAND 1     // 0=exclude, 1=include, reboot included by default
#define TELNET_DMESG_COMMAND 1      // 0=exclude, 1=include, dmesg included by default
#define TELNET_QUIT_COMMAND 1       // 0=exclude, 1=include, quit included by default
#define TELNET_UPTIME_COMMAND 1     // 0=exclude, 1=include, date included by default
#define TELNET_DATE_COMMAND 1       // 0=exclude, 1=include, date included by default
#define TELNET_NTPDATE_COMMAND 1    // 0=exclude, 1=include, ntpdate included by default
#define TELNET_PING_COMMAND 1       // 0=exclude, 1=include, ping included by default
#define TELNET_IFCONFIG_COMMAND 1   // 0=exclude, 1=include, ifconfig included by default
#define TELNET_IW_COMMAND 0         // 0=exclude, 1=include, iw included by default
#define TELNET_NETSTAT_COMMAND 0    // 0=exclude, 1=include, netstat included by default
#define TELNET_KILL_COMMAND 0       // 0=exclude, 1=include, kill included by default
#define TELNET_CURL_COMMAND 0       // 0=exclude, 1=include, curl included by default
#define TELNET_SENDMAIL_COMMAND 0   // 0=exclude, 1=include, sendmail included by default
#define TELNET_LS_COMMAND 0         // 0=exclude, 1=include, ls included by default
#define TELNET_TREE_COMMAND 0       // 0=exclude, 1=include, tree included by default
#define TELNET_MKDIR_COMMAND 0      // 0=exclude, 1=include, mkdir included by default
#define TELNET_RMDIR_COMMAND 0      // 0=exclude, 1=include, rmdir included by default
#define TELNET_CD_COMMAND 0         // 0=exclude, 1=include, cd included by default
#define TELNET_PWD_COMMAND 0        // 0=exclude, 1=include, pwd included by default
#define TELNET_CAT_COMMAND 0        // 0=exclude, 1=include, cat included by default
#define TELNET_VI_COMMAND 0         // 0=exclude, 1=include, vi included by default
#define TELNET_CP_COMMAND 0         // 0=exclude, 1=include, cp included by default
#define TELNET_RM_COMMAND 0         // 0=exclude, 1=include, rm included by default
#define TELNET_LSOF_COMMAND 0       // 0=exclude, 1=include, lsof included by default

#define SWAP_DEL_AND_BACKSPACE 0    // seto to 1 to swap the meaning of these keys - this would be suitable for Putty and Linux Telnet clients

#include <telnetServer.h>

telnetServer_t *telnetServer = NULL;

void setup () {
  Serial.begin (115200);

  // 2️⃣ Start WiFi connection
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");

  // 3️⃣  Create Telnet server instance
  telnetServer = new (std::nothrow) telnetServer_t ();  // optional arguments:
                                                        // Cstring<255> (*__getUserHomeDirectory__) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                                        // String (*telnetCommandHandlerCallback) (int argc, char *argv [], telnetConnection_t *tcn) = NULL
                                                        // int serverPort = 23
                                                        // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                                        // bool runListenerInItsOwnTask = true
                                                        
  // 4️⃣ Check if Telnet server instance is created && Telnet server is running
  if (telnetServer && *telnetServer)
    Serial.println ("Telnet server started");
  else
    Serial.println ("Telnet server did not start");

  // 5️⃣ Use Telent client to connect to ESP32's IP address
  while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) { // wait until we get IP from router's DHCP
      delay (1000); 
      Serial.println ("   ."); 
  } 
  Serial.print ("Got IP addess: "); Serial.println (WiFi.localIP ());

  // 6️⃣ Setting the time is only important for time commands
  setenv ("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // or select another (POSIX) time zones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  ntpClient_t ntpClient ("1.si.pool.ntp.org", "2.si.pool.ntp.org", "3.si.pool.ntp.org");
  ntpClient.syncTime ();


  // ...
}

void loop () {

}
