/*
  **Note:** Each example demonstrates only a specific feature or use case.  
  The complete, fully integrated server solution is available here:  
  https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino/blob/master/PROJECT_STATE.md
*/


#include <WiFi.h>
#include <LittleFS.h>               // Or SPIFFS.h or FFat.h or SD.h ...
#include <threadSafeFS.h>
#include <Cstring.hpp>
threadSafeFS::FS TSFS (LittleFS);   // Or SPIFFS or FFat or SD ...   
using File = threadSafeFS::File;    // Use thread-safe wrapper for all file operations from now on in your code


// Choose which built-in Telnet commands will be included
#define TELNET_CLEAR_COMMAND 0      // 0=exclude, 1=include, clear is included by default
#define TELNET_UNAME_COMMAND 0      // 0=exclude, 1=include, uname is included by default
#define TELNET_FREE_COMMAND 0       // 0=exclude, 1=include, free is included by default
#define TELNET_NOHUP_COMMAND 0      // 0=exclude, 1=include, nohup is included by default
#define TELNET_REBOOT_COMMAND 0     // 0=exclude, 1=include, reboot is included by default
#define TELNET_DMESG_COMMAND 0      // 0=exclude, 1=include, dmesg is included by default
#define TELNET_QUIT_COMMAND 0       // 0=exclude, 1=include, quit is included by default
#define TELNET_UPTIME_COMMAND 0     // 0=exclude, 1=include, date is included by default
#define TELNET_DATE_COMMAND 0       // 0=exclude, 1=include, date is included by default
#define TELNET_NTPDATE_COMMAND 0    // 0=exclude, 1=include, ntpdate is included by default
#define TELNET_CRONTAB_COMMAND 0    // 0=exclude, 1=include, crontab is included by default
#define TELNET_PING_COMMAND 0       // 0=exclude, 1=include, ping is included by default
#define TELNET_IFCONFIG_COMMAND 0   // 0=exclude, 1=include, ifconfig is included by default
#define TELNET_IW_COMMAND 0         // 0=exclude, 1=include, iw is included by default
#define TELNET_NETSTAT_COMMAND 0    // 0=exclude, 1=include, netstat is included by default
#define TELNET_KILL_COMMAND 0       // 0=exclude, 1=include, kill is included by default
#define TELNET_CURL_COMMAND 0       // 0=exclude, 1=include, curl is included by default
#define TELNET_SENDMAIL_COMMAND 0   // 0=exclude, 1=include, sendmail is included by default
#define TELNET_LS_COMMAND 1         // 0=exclude, 1=include, ls is included by default
#define TELNET_TREE_COMMAND 1       // 0=exclude, 1=include, tree is included by default
#define TELNET_MKDIR_COMMAND 1      // 0=exclude, 1=include, mkdir is included by default
#define TELNET_RMDIR_COMMAND 1      // 0=exclude, 1=include, rmdir is included by default
#define TELNET_CD_COMMAND 1         // 0=exclude, 1=include, cd is included by default
#define TELNET_PWD_COMMAND 1        // 0=exclude, 1=include, pwd is included by default
#define TELNET_CAT_COMMAND 1        // 0=exclude, 1=include, cat is included by default
#define TELNET_VI_COMMAND 1         // 0=exclude, 1=include, vi is included by default
#define TELNET_CP_COMMAND 1         // 0=exclude, 1=include, cp is included by default
#define TELNET_RM_COMMAND 1         // 0=exclude, 1=include, rm is included by default
#define TELNET_LSOF_COMMAND 1       // 0=exclude, 1=include, lsof is included by default

#define SWAP_DEL_AND_BACKSPACE 0    // set to 1 to swap the meaning of these keys - this would be suitable for Putty and Linux Telnet clients


// 1️⃣ Provide help text for user commands
#define USER_DEFINED_TELNET_HELP_TEXT   "\r\n  user management:" \
                                        "\r\n       useradd -d <userHomeDirectory> <userName>" \
                                        "\r\n       userdel <userName>" \
                                        "\r\n       passwd [<userName>]"

#include <telnetServer.h>
telnetServer_t *telnetServer = NULL;


// 2️⃣ Include userManagement_t class
//     Create default /etc/passwd and /etc/shadow (with root/rootpassword and webadmin/webadminpassword) it they don't exist yet
#include "userManagement.h"
userManagement_t *userManagement;


// 3️⃣ Provide login callback function
// returns        "/" for full access
// something like "/home/name" for limited access
//                "" for no access
Cstring<255> getUserHomeDirectoryCallback (const Cstring<64>& userName, const Cstring<64>& password) {

    // Must be reentrant !!!


    Cstring<255> retVal;
    // check if userName and password are correct
    if (userManagement && userManagement->checkUserNameAndPassword (userName, password))
        return userManagement->getHomeDirectory (userName);
    // else
    return "";  // access (login) denyed
}


// 4️⃣ Provide Telnet (uswer defined) command handler
String telnetCommandHandlerCallback (int argc, char *argv [], telnetServer_t::telnetConnection_t *tcn) {

    // Must be reentrant !!!
    

    #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))  
    #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
    
    if (argv0is ("useradd"))        { 
                                        if (strcmp (tcn->getUserName (), "root"))       return "Only root may add users";
                                        if (argc == 4 && argv1is ("-d"))                return userManagement->userAdd (argv [3], argv [2]);
                                                                                        return "Wrong syntax, use useradd -d userHomeDirectory userName";
                                    }
                                    
    else if (argv0is ("userdel"))   {
                                            if (strcmp (tcn->getUserName (), "root"))   return "Only root may delete users";
                                            if (argc != 2)                              return "Wrong syntax. Use userdel userName";
                                            if (!strcmp (argv [1], "root"))             return "You don't really want to to this";
                                                                                        return userManagement->userDel (argv [1]);
                                    }

    else if (argv0is ("passwd"))    {
                                            const char *forUser = tcn->getUserName (); // usually, but may be changed later
                                            if (argc == 1) { // user changing password for himself
                                                // read current password
                                                tcn->dontEcho ();
                                                if (tcn->sendString ("Enter current password: ") <= 0) 
                                                    return "\r"; 
                                                char password [USER_PASSWORD_MAX_LENGTH + 1];
                                                if (tcn->recvLine (password, sizeof (password), true) != 13) { // the line is not ended with Enter
                                                    tcn->doEcho ();                  
                                                    return "\r\nPassword not changed"; 
                                                }
                                                tcn->doEcho (); 
                                                // check if password is valid for user
                                                if (!userManagement->checkUserNameAndPassword (tcn->getUserName (), password))
                                                    return "Wrong password";                                                     
                                            }
                                            if (argc == 2) {
                                                forUser = argv [1];
                                                if (!strcmp (tcn->getUserName (), argv [1])) { // user changing password for himself
                                                    // read current password
                                                    tcn->dontEcho ();
                                                    if (tcn->sendString ("Enter current password: ") <= 0) 
                                                        return "\r";
                                                    char password [USER_PASSWORD_MAX_LENGTH + 1];
                                                    if (tcn->recvLine (password, sizeof (password), true) != 13) { // the line is not ended with Enter
                                                        tcn->doEcho ();                  
                                                        return "\r\nPassword not changed"; 
                                                    }
                                                    tcn->doEcho (); 
                                                    // check if password is valid for user
                                                    if (!userManagement->checkUserNameAndPassword (argv [1], password))
                                                        return "Wrong password";                                                     
                                                } else if (!strcmp (tcn->getUserName (), "root")) { // root is changing password for another user
                                                    // check if user exists with getUserHomeDirectory
                                                    Cstring<255> homeDirectory = userManagement->getHomeDirectory (argv [1]);
                                                    if (homeDirectory == "")       
                                                        return "User does not exist";
                                                } else {
                                                    return "You may change only your own password";
                                                }
                                            }

                                            // it is OK to change the password now, read it twice
                                            char password1 [USER_PASSWORD_MAX_LENGTH + 1];
                                            char password2 [USER_PASSWORD_MAX_LENGTH + 1];
                                            tcn->dontEcho ();
                                            if (tcn->sendString ("\r\nEnter new password: ") <= 0)
                                                return "\r"; 
                                            if (tcn->recvLine (password1, sizeof (password1), true) != 13) { // the line is not ended with Enter
                                                tcn->doEcho ();
                                                return "\r\nPassword not changed"; 
                                            }
                                            if (tcn->sendString ("\r\nRe-enter new password: ") <= 0)
                                                return "\r"; 
                                            if (tcn->recvLine (password2, sizeof (password2), true) != 13) { // the line is not ended with Enter
                                                tcn->doEcho ();
                                                return "\r\nPassword not changed"; 
                                            }
                                            tcn->doEcho ();
                                            // check passwords
                                            if (strcmp (password1, password2))
                                                return "\r\nPasswords do not match";
                                            // change password
                                            if (userManagement->passwd (forUser, password1))
                                                return "\r\nPassword changed";
                                            else
                                                return "\r\nError changing password";  
                                    }

    // let the Telnet server handle command itself
    return "";
}


void setup () {
    Serial.begin (115200);


    // 5️⃣ Mount file system and create thread-safe file system wrapper arround it (usernames and password are stored in files)
    LittleFS.begin (true);  // Or SPIFFS or FFat or SD ...


    // 6️⃣ Create userManagement instance
    userManagement = new (std::nothrow) userManagement_t (TSFS);
    

    WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");


    // 7️⃣ Create Telnet server instance with user-defined callback functions for login and handling commands
    telnetServer = new (std::nothrow) telnetServer_t (TSFS, getUserHomeDirectoryCallback, telnetCommandHandlerCallback);  // optional arguments:
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
