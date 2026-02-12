#include <WiFi.h>
#include <LittleFS.h>
#include <threadSafeFS.h>
#include <Cstring.hpp>


// 1️⃣ Include thread-safe file system (usernames and password are stored in files)
threadSafeFS::FS TSFS (LittleFS);
using File = threadSafeFS::File;  // Use thread-safe wrapper for all file operations form now on in your code


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


// 2️⃣ Provide help text for user commands
#define USER_DEFINED_TELNET_HELP_TEXT   "\r\n  user management:" \
                                        "\r\n       useradd -u <userId> -d <userHomeDirectory> <userName>" \
                                        "\r\n       userdel <userName>" \
                                        "\r\n       passwd [<userName>]"

#include <telnetServer.h>
telnetServer_t *telnetServer = NULL;


// 3️⃣ Create Unix-like user management class and instance
// TUNNING PARAMETERS
#define USER_PASSWORD_MAX_LENGTH 64         // the number of characters of longest user name or password 
#define MAX_ETC_PASSWD_SIZE (1024 + 256)    // 1 KB will usually do - some functions read the whole /etc/passwd file into the memory 
#define MAX_ETC_SHADOW_SIZE (1024 + 256)    // 1 KB will usually do - some functions read the whole /etc/shadow file into the memory
#define DEFAULT_ROOT_PASSWORD "rootpassword"
#define DEFAULT_WEBADMIN_PASSWORD "webadminpassword"
#define DEFAULT_USER_PASSWORD "changeimmediatelly"

#include <mbedtls/md.h>

class userManagement_t {

    public:

        // creates user management files (if they don't exist yet) with root, webadmin, webserver and telnetserver users, if they don't exist - only 3 fields are used: user name, hashed password and home directory
        void initialize () { 
            // create dirctory structure
            if (!TSFS.isDirectory ("/etc"))
                TSFS.mkdir ("/etc"); 
            // create /etc/passwd if it doesn't exist
            if (!TSFS.isFile ("/etc/passwd")) {
                bool created = false;
                File f = TSFS.open ("/etc/passwd", "w");
                if (f) {
                    created = fprintf (f, "root:x:0:::/:\r\n" "webadmin:x:1000:::/var/www/html/:\r\n") > 0;                            
                    f.close ();
                }
                cout << (created ? "/etc/passwd created\n" : "error creating /etc/passwd\n");
            }
            // create /etc/shadow if it doesn't exist
            if (!TSFS.isFile ("/etc/shadow")) {
                bool created = false;
                File f = TSFS.open ("/etc/shadow", "w");
                if (f) {
                    char rootpassword [USER_PASSWORD_MAX_LENGTH + 1]; 
                    __sha256__ (rootpassword, sizeof (rootpassword), DEFAULT_ROOT_PASSWORD);
                    char webadminpassword [USER_PASSWORD_MAX_LENGTH + 1]; 
                    __sha256__ (webadminpassword, sizeof (webadminpassword), DEFAULT_WEBADMIN_PASSWORD);
                    created = fprintf (f, "root:$5$%s:::::::\r\n" "webadmin:$5$%s:::::::\r\n", rootpassword, webadminpassword) > 0;
                    f.close ();
                }
                cout << (created ? "/etc/shadow\n" : "error creating /etc/shadow\n");
            }
        }

        // scan through /etc/shadow file for (user name, pasword) pair and return true if found
        bool checkUserNameAndPassword (const char *userName, const char *password) { 
            // initial checking
            if (strlen (userName) > USER_PASSWORD_MAX_LENGTH || strlen (password) > USER_PASSWORD_MAX_LENGTH) 
                return false;

            char srchStr [USER_PASSWORD_MAX_LENGTH + 65 + 6];
            sprintf (srchStr, "%s:$5$", userName);
            __sha256__ (srchStr + strlen (srchStr), 65, password);
            strcat (srchStr, ":"); // we get something like "root:$5$de362bbdf11f2df30d12f318eeed87b8ee9e0c69c8ba61ed9a57695bbd91e481:"
            File f = TSFS.open ("/etc/shadow", "r"); 
            if (f) { 
                char line [USER_PASSWORD_MAX_LENGTH + 65 + 6]; // this should do, we don't even need the whole line which looks something like "root:$5$de362bbdf11f2df30d12f318eeed87b8ee9e0c69c8ba61ed9a57695bbd91e481:::::::"
                int i = 0;
                while (f.available ()) {
                    char c = f.read ();
                    if (c >= ' ' && i < sizeof (line) - 1) {
                        line [i++] = c; // append line 
                    } else {
                        line [i] = 0; i = 0; // terminate the line string
                        i = 0; // the next time start from beginning of the line
                        if (strstr (line, srchStr) == line) { // the beginning of line matches with user name and sha of password
                            f.close ();
                            return true; // success
                        }
                    }
                }
                f.close ();
            }
            return false;
        } 

        // returns        "/" for full access
        // something like "/home/name" for limited access
        //                "" for no access
        Cstring<255> getHomeDirectory (const char *userName) {
            Cstring<255> retVal;
            if (strlen (userName) > USER_PASSWORD_MAX_LENGTH)
                return retVal;
            
            char srchStr [USER_PASSWORD_MAX_LENGTH + 2]; 
            sprintf (srchStr, "%s:", userName);
            File f = TSFS.open ("/etc/passwd", "r");
            if (f) { 
                char line [USER_PASSWORD_MAX_LENGTH + 32 + 255 + 1]; // this should do for one line which looks something like "webserver::100:::/var/www/html/:"
                int i = 0;
                while (f.available ()) {
                    char c = f.read ();
                    if (c >= ' ' && i < sizeof (line) - 1) line [i++] = c; 
                    else {
                        line [i] = 0; // terminate the line string
                        i = 0; // the next time start from beginning of the line
                        if (strstr (line, srchStr) == line) { // userName matches
                            char *homeDirectory = strstr (line, ":::");
                            if (homeDirectory) {
                                char format [16]; 
                                sprintf (format, ":::%%%i[^:]", retVal.max_size ()); // we get something like ":::%255[^:]"
                                if (sscanf (homeDirectory, format, retVal.c_str ()) != 1)
                                    retVal = "";
                            }
                        }
                    }
                }
                f.close ();
            }
            return retVal;
        }    

        // bool passwd (userName, newPassword) assignes a new password for the user by writing it's SHA value into /etc/shadow file, return success
        // __ignoreIfUserDoesntExist__ and __dontWriteNewPassword__ arguments are only used internaly by userDel function
        bool passwd (const char *userName, const char *newPassword, bool __ignoreIfUserDoesntExist__ = false, bool __dontWriteNewPassword__ = false) {
            if (strlen (newPassword) > USER_PASSWORD_MAX_LENGTH) 
                return false;   
            
            // read /etc/shadow
            char buffer [MAX_ETC_SHADOW_SIZE + 3]; 
            buffer [0] = '\n'; 
            int i = 1;
            File f = TSFS.open ("/etc/shadow", "r"); 
            if (f) { 
                while (f.available () && i <= MAX_ETC_SHADOW_SIZE) 
                    if ((buffer [i] = f.read ()) != '\r') 
                        i++; 
                buffer [i++] = '\n'; 
                buffer [i] = 0; // read the whole file into C string ignoring \r
                if (f.available ()) { 
                    f.close (); 
                    return false; // /etc/shadow too large
                }
                f.close ();
            } else { 
                return false; // can't read /etc/shadow
            }

            // find user's record in the buffer
            char srchStr [USER_PASSWORD_MAX_LENGTH + 6];
            sprintf (srchStr, "\n%s:$5$", userName); // we get something like \nroot:$5$
            char *u = strstr (buffer, srchStr);
            if (!u && !__ignoreIfUserDoesntExist__) 
                return false; // user not found in /etc/shadow

            // write all the buffer except user's record back to /etc/shadow and then add a new record for the user
            f = TSFS.open ("/etc/shadow", "w");
            if (f) {
                char *lineBeginning = buffer + 1;
                char *lineEnd;
                while ((lineEnd = strstr (lineBeginning, "\n"))) {
                    *lineEnd = 0;
                    if (lineBeginning != u + 1 && lineEnd - lineBeginning > 13) { // skip user's record and empty (<= 13) records
                        if (fprintf (f, "%s\r\n", lineBeginning) < strlen (lineBeginning) + 2) { 
                            close (f); 
                            return false; // can't write /etc/shadow - this is bad because we have already corrupted it :/
                        } 
                    }
                    lineBeginning = lineEnd + 1;
                }
                if (!__dontWriteNewPassword__) {
                    // add user's record
                    strcpy (buffer, srchStr + 1);
                    __sha256__ (buffer + strlen (buffer), sizeof (buffer) - strlen (buffer), newPassword);
                    if (fprintf (f, "%s:::::::\r\n", buffer) < strlen ("%s:::::::\r\n")) { 
                        close (f); 
                        return false; // can't write /etc/shadow - this is bad because we have already corrupted it :/
                    }
                }     
                f.close (); 
            } else {
                return false; // can't write /etc/shadow
            }
            return true; // success      
        }

        // char *userAdd (userName, userId, userHomeDirectory) adds userName, userId, userHomeDirectory to /etc/passwd and /etc/shadow, creates home directory and returns success or error message
        // __ignoreIfUserExists__ and __dontWriteNewUser__ arguments are only used internaly by userDel function
        const char *userAdd (const char *userName, const char *userId, const char *userHomeDirectory, bool __ignoreIfUserExists__ = false, bool __dontWriteNewUser__ = false) {
            if (strlen (userName) < 1)                          return "Missing user name";
            if (strlen (userName) > USER_PASSWORD_MAX_LENGTH)   return "User name too long";
            if (strstr (userName, ":"))                         return "User name may not contain ':' character";
            if (atoi (userId) <= 0)                             return "User id should be > 0"; // this information is not really used, 0 is reserverd for root in case of further development
            if (strlen (userHomeDirectory) < 1)                 return "Missing user's home directory";
            if (strlen (userHomeDirectory) > 255)               return "User's home directory name too long";

            Cstring<255> homeDirectory = userHomeDirectory;

            // read /etc/passwd
            {
                char buffer [MAX_ETC_PASSWD_SIZE + 3];
                buffer [0] = '\n';
                int i = 1;
                File f = TSFS.open ("/etc/passwd", "r");
                if (f) { 
                    while (f.available () && i <= MAX_ETC_PASSWD_SIZE) 
                        if ((buffer [i] = f.read ()) != '\r') 
                            i++; 
                    buffer [i++] = '\n'; 
                    buffer [i] = 0; // read the whole file in C string ignoring \r
                    if (f.available ()) { 
                        f.close (); 
                        return "/etc/shadow is too large"; 
                    } 
                    f.close ();
                } else {
                    return "Can't read /etc/passwd"; 
                }
                // find user's record in the buffer
                char srchStr [USER_PASSWORD_MAX_LENGTH + 3];
                sprintf (srchStr, "\n%s:", userName); // we get something like \nroot:
                char *u = strstr (buffer, srchStr);
                if (u && !__ignoreIfUserExists__) 
                    return "User with this name already exists";
                if (!__dontWriteNewUser__ && strlen (buffer) + strlen (userName) + strlen (userId) + strlen (homeDirectory) + 10 > MAX_ETC_PASSWD_SIZE) 
                    return "Can't add a user because /etc/passwd file is already too long";

                // write all the buffer back to /etc/passwd and then add a new record for the new user
                f = TSFS.open ("/etc/passwd", "w");
                if (f) {
                    char *lineBeginning = buffer + 1;
                    char *lineEnd;
                    while ((lineEnd = strstr (lineBeginning, "\n"))) {
                        *lineEnd = 0;
                        if (lineBeginning != u + 1 && lineEnd - lineBeginning > 2) { // skip skip user's record and empty (<= 2) records
                            if (fprintf (f, "%s\r\n", lineBeginning) < strlen (lineBeginning) + 2) { 
                                close (f); 
                                return "Can't write /etc/passwd"; // this is bad because we have already corrupted it :/
                            } 
                        }
                        lineBeginning = lineEnd + 1;
                    }
                    if (!__dontWriteNewUser__) {
                        // add user's record
                        strcpy (buffer, srchStr + 1);
                        if (fprintf (f, "%s:x:%s:::%s:\r\n", buffer, userId, homeDirectory) < strlen ("%s:x:%s:::%s:\r\n")) { 
                            close (f); 
                            return "Can't write /etc/passwd"; // this is bad because we have already corrupted it :/
                        }
                    }         
                    f.close (); 
                } else { 
                    return "Can't write /etc/passwd";
                }
            }

            if (__dontWriteNewUser__) 
                return "User deleted";
            // write password in /etc/shadow file
            if (!passwd (userName, DEFAULT_USER_PASSWORD, true)) {
                userDel (userName); // try to roll-back the changes we made so far 
                return "Can't write /etc/shadow";
            } 
            // crate user's home directory
            if (homeDirectory [homeDirectory.length () - 1] != '/')
                homeDirectory += '/';
            bool b = false;
            for (int i = 0; homeDirectory [i] && homeDirectory [i]; i++) 
                if (i && homeDirectory [i] == '/') {
                    char tmp = homeDirectory [i];
                    homeDirectory [i] = 0; 
                    b = TSFS.mkdir (homeDirectory); 
                    homeDirectory [i] = tmp;
                }
            if (!b) 
                return "User created with default password '" DEFAULT_USER_PASSWORD "' but couldn't create user's home directory";
                
            return "User created with default password '" DEFAULT_USER_PASSWORD "'";
        }

        // char *userDel (userName) deletes userName from /etc/passwd and /etc/shadow, deletes home directory and returns "" for success or error message
        const char *userDel (const char *userName) {
            if (!TSFS.mounted ()) 
                return "File system not mounted";
        
            // delete user's password from /etc/shadow file
            if (!passwd (userName, "", true, true)) 
                return "Can't write /etc/shadow";

            // get user's home directory from /etc/passwd
            Cstring<255> homeDirectory = getHomeDirectory (userName);
            // delete user from /etc/passwd
            if (strcmp (userAdd (userName, "32767", "$", true, true), "User deleted")) 
                return "Can't write /etc/passwd";

            // remove user's home directory
            bool homeDirectoryRemoved = false; 
            if (homeDirectory != "") {
                bool firstDirectory = true;
                for (int i = strlen (homeDirectory); i; i--) {
                    if (homeDirectory [i] == 0 || homeDirectory [i] == '/') { 
                        homeDirectory [i] = 0; 
                        if (!TSFS.rmdir (homeDirectory)) 
                            break;
                        if (firstDirectory) 
                            homeDirectoryRemoved = true;
                        firstDirectory = false;
                    }
                }
                // if we've got home directory we can assume that the user existed prior to calling userDel
                if (homeDirectoryRemoved)
                    return "User deleted";
                else
                    return "User deleted but couldn't remove user's home directory";
            } else {
                // if we haven't got home directory we can assume that the user did not exist prior to calling userDel, at least not regurally, but we have corrected /etc/passwd and /etc/shadow now
                return "User does not exist";
            }
        }


    private:

        bool __sha256__ (char *buffer, size_t bufferSize, const char *clearText) { // converts clearText to 256 bit SHA, returns character representation in hexadecimal format of hash value
            *buffer = 0;
            byte shaByteResult [32];
            char shaCharResult [65];
            mbedtls_md_context_t ctx;
            mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
            mbedtls_md_init (&ctx);
            mbedtls_md_setup (&ctx, mbedtls_md_info_from_type (md_type), 0);
            mbedtls_md_starts (&ctx);
            mbedtls_md_update (&ctx, (const unsigned char *) clearText, strlen (clearText));
            mbedtls_md_finish (&ctx, shaByteResult);
            mbedtls_md_free (&ctx);
            for (int i = 0; i < 32; i++) 
                sprintf (shaCharResult + 2 * i, "%02x", (int) shaByteResult [i]);
            if (bufferSize > strlen (shaCharResult)) { 
                strcpy (buffer, shaCharResult); 
                return true; 
            }
            return false; 
        }

};                                                                                    

// crete working instance
userManagement_t userManagement;


// 4️⃣ Provide login callback function
Cstring<255> getUserHomeDirectoryCallback (const Cstring<64>& userName, const Cstring<64>& password) {
    Cstring<255> retVal;
    // check if userName and password are correct
    if (userManagement.checkUserNameAndPassword (userName, password))
        return userManagement.getHomeDirectory (userName);
    // else
    return "";  // access (login) denyed
}


// 5️⃣ Provide Telnet (uswer defined) command handler
String telnetCommandHandlerCallback (int argc, char *argv [], telnetServer_t::telnetConnection_t *tcn) {

    #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))  
    #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
    #define argv2is(X) (argc > 2 && !strcmp (argv[2], X))
    #define argv3is(X) (argc > 3 && !strcmp (argv[3], X))    
    
    if (argv0is ("useradd"))        { 
                                        if (strcmp (tcn->getUserName (), "root"))           return "Only root may add users";
                                        if (argc == 6 && argv1is ("-u") && argv3is ("-d"))  return userManagement.userAdd (argv [5], argv [2], argv [4]);
                                                                                            return "Wrong syntax, use useradd -u userId -d userHomeDirectory userName (where userId > 1000)";
                                    }
                                    
    else if (argv0is ("userdel"))   {
                                            if (strcmp (tcn->getUserName (), "root"))   return "Only root may delete users";
                                            if (argc != 2)                              return "Wrong syntax. Use userdel userName";
                                            if (!strcmp (argv [1], "root"))             return "You don't really want to to this";
                                                                                        return userManagement.userDel (argv [1]);
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
                                                if (!userManagement.checkUserNameAndPassword (tcn->getUserName (), password))
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
                                                    if (!userManagement.checkUserNameAndPassword (argv [1], password))
                                                        return "Wrong password";                                                     
                                                } else if (!strcmp (tcn->getUserName (), "root")) { // root is changing password for another user
                                                    // check if user exists with getUserHomeDirectory
                                                    Cstring<255> homeDirectory = userManagement.getHomeDirectory (argv [1]);
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
                                            if (userManagement.passwd (forUser, password1))
                                                return "\r\nPassword changed";
                                            else
                                                return "\r\nError changing password";  
                                    }

    // let the Telnet server handle command itself
    return "";
}


void setup () {
  Serial.begin (115200);
  WiFi.begin ("YOUR_SSID", "YOUR_PASSWORD");
  LittleFS.begin (true);

  // 6️⃣ Create default /etc/passwd and /etc/shadow (with root/rootpassword and webadmin/webadminpassword) it they don't exist yet
  userManagement.initialize ();
  
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
