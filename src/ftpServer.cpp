/*

    ftpServer.hpp

    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


    Dec 25, 2025, Bojan Jurca


    Classes implemented/used in this module:

        ftpServer_t
        ftpServer_t::ftpControlConnection_t
        tcpClient_t for active data connection
        tcpServer_t, tcpConnection_t for pasive data connection

    Inheritance diagram:

             ┌─────────────┐
             │ tcpServer_t ┼┐
             └─────────────┘│
                            │
      ┌─────────────────┐   │
      │ tcpConnection_t ┼┐  │
      └─────────────────┘│  │
                         │  │
                         │  │  ┌─────────────────────────┐
                         │  ├──┼─ httpServer_t           │
                         ├─────┼──── webSocket_t         │
                         │  │  │     └── httpConection_t │
                         │  │  └─────────────────────────┘
                         │  │  logicly webSocket_t would inherit from httpConnection_t but it is easier to implement it the other way around
                         │  │
                         │  │
                         │  │  ┌────────────────────────┐
                         │  ├──┼─ telnetServer_t        │
                         ├─────┼──── telnetConnection_t │
                         │  │  └────────────────────────┘
                         │  │
                         │  │
                         │  │  ┌────────────────────────────┐
                         │  └──┼─ ftpServer_t               │
                         ├─────┼──── ftpControlConnection_t │
                         │     └────────────────────────────┘
                         │  
                         │  
                         │     ┌──────────────┐
                         └─────┼─ tcpClient_t │
                               └──────────────┘


    Nomenclature used here for easier understaning of the code:

     - "connection" normally applies to TCP connection from when it was established to when it is terminated

                  There is normally only one control TCP connection per FTP session. Beside control connection FTP client and server open
                  also a data TCP connection when certain commands are involved (like LIST, RETR, STOR, ...).

     - "session" applies to user interaction between login and logout

                  The first thing that the user does when a TCP connection is established is logging in and the last thing is logging out. It TCP
                  connection drops for some reason the user is automatically logged out.

      - "buffer size" is the number of bytes that can be placed in a buffer. 
      
                  In case of C 0-terminated strings the terminating 0 character should be included in "buffer size".

    Handling FTP commands that always arrive through control TCP connection is pretty straightforward. The data transfer TCP connection, on the other 
    hand, can be initialized eighter by the server or the client. In the first case, the client starts a listener (thus acting as a server) and sends 
    its connection information (IP and port number) to the FTP server through the PORT command. The server then initializes data connection to the 
    client. This is called active data connection. Windows FTP.exe uses this kind of data transfer by default. In the second case, the client sends 
    a PASV command to the FTP server, then the server starts another listener (beside the control connection listener that is already running) and 
    sends its connection information (IP and port number) back to the client as a reply to the PASV command. The client then initializes data 
    connection to the server. This is called passive data connection. Windows Explorer uses this kind of data transfer.

*/


#include <WiFi.h>
#include "ftpServer.h"
#include <errno.h>
#include <string.h>


// static member initialization
UBaseType_t ftpServer_t::ftpControlConnection_t::__lastHighWaterMark__ = FTP_CONTROL_CONNECTION_STACK_SIZE;


// ----- ftpControlConnection_t implementation -----

ftpServer_t::ftpControlConnection_t::ftpControlConnection_t (threadSafeFS::FS fileSystem,
                                                             Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName,
                                                             const Cstring<64>& password),
                                                             int connectionSocket,
                                                             char *clientIP,
                                                             char *serverIP) : tcpConnection_t (connectionSocket, clientIP, serverIP),
                                                                               __fileSystem__ (fileSystem),
                                                                               __getUserHomeDirectory__ (getUserHomeDirectory) {
}

ftpServer_t::ftpControlConnection_t::~ftpControlConnection_t () {
    __closeDataConnection__ (); // only if still opened
}

void ftpServer_t::ftpControlConnection_t::__runConnectionTask__ () {
    // this is where FTP control connection starts, the socket is already opened

    if (sendString ("220-" HOSTNAME " FTP server - please login\r\n220 \r\n") <= 0)
        return;

    while (true) {

        // 1. read the request
        switch (recvString (__cmdLine__, FTP_CMDLINE_BUFFER_SIZE, "\n")) {
            case -1:
            case 0:                         return;
            case FTP_CMDLINE_BUFFER_SIZE:
                                            getLogQueue () << "[ftpCtrlConn] buffer too small" << endl;
                                            return;
            default:                        break;
        }

        // 2. parse command line
        int argc = 0;
        char *argv [FTP_SESSION_MAX_ARGC];

        char *q = __cmdLine__ - 1;
        while (true) {
            char *p = q + 1;
            while (*p && *p <= ' ')
                p++;
            if (*p) {
                bool quotation = false;
                if (*p == '\"') {
                    quotation = true;
                    p++;
                }
                argv [argc++] = p;
                q = p;
                while (*q && (*q > ' ' || quotation))
                    if (*q == '\"')
                        break;
                    else
                        q++;
                if (*q)
                    *q = 0;
                else
                    break;
            } else {
                break;
            }
            if (argc == FTP_SESSION_MAX_ARGC)
                break;
        }

        // 3. process the commandLine
        if (argc) {
            Cstring<300> s = __internalCommandHandler__ (argc, argv);
            if (s != "") {
                if (sendString (s) <= 0)
                    goto endConnection;
            }

            if (strstr ((char *) s, "221") == (char *) s)
                goto endConnection;
        }

        // check how much o stack did we use
        UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
        if (__lastHighWaterMark__ > highWaterMark) {
            getLogQueue () << "[ftpCtrlConn] new FTP connection stack high water mark reached: " << highWaterMark << " not used bytes" << endl;
            __lastHighWaterMark__ = highWaterMark;
        }
    }

endConnection:
    getLogQueue () << "[ftpCtrlConn] " << __userName__ << " logged out" << endl;
}

Cstring<300> ftpServer_t::ftpControlConnection_t::__internalCommandHandler__ (int argc, char *argv []) {
    #define ftpArgv0Is(X) (argc > 0 && !strcmp(argv[0], X))
    #define ftpArgv1Is(X) (argc > 1 && !strcmp(argv[1], X))
    #define ftpArgv2Is(X) (argc > 2 && !strcmp(argv[2], X))

    // for (int i = 0; i < argc; i++) log_i (" %s", argv [i]); log_i ("\n");

    if (ftpArgv0Is ("QUIT"))                            return "221 closing connection\r\n";

    else if (ftpArgv0Is ("OPTS"))                       if (argc == 3 && ftpArgv1Is ("UTF8") && ftpArgv2Is ("ON"))
                                                            return "200 UTF8 enabled\r\n";
                                                        else
                                                            return "502 OPTS arguments not supported\r\n";

    else if (ftpArgv0Is ("USER"))                       return __USER__(argv [1]);

    else if (ftpArgv0Is ("PASS"))                       return __PASS__ (argv [1]);
    else if (ftpArgv0Is ("PWD") || ftpArgv0Is ("XPWD")) return __XPWD__ ();

    else if (ftpArgv0Is ("TYPE"))                       return "200 ok\r\n";

    else if (ftpArgv0Is ("NOOP"))                       return "200 ok\r\n";

    else if (ftpArgv0Is ("SYST"))                       return "215 UNIX Type: L8\r\n";

    else if (ftpArgv0Is ("FEAT"))                       return "211-Extensions supported:\r\n UTF8\r\n211 end\r\n";

    else if (ftpArgv0Is ("PORT"))                       return __PORT__ (argv [1]);

    else if (ftpArgv0Is ("EPRT"))                       return __EPRT__ (argv [1]);

    else if (ftpArgv0Is ("PASV"))                       return __PASV__ ();

    else if (ftpArgv0Is ("EPSV"))                       return __EPSV__ ();

    // reconstruct (part of) command line (if it contains long file name)
    if (argc > 1) {
        for (char *p = argv [1]; p < argv [argc - 1]; p++)
            if (!*p)
                *p = ' ';
    } else {
        argv [1] = (char *) "";
    }

    if (ftpArgv0Is ("LIST") || ftpArgv0Is ("NLST"))                             return __NLST__(argc > 1 ? argv [1] : (char *) __workingDirectory__);

    else if (ftpArgv0Is ("SIZE"))                                               return __SIZE__ (argv [1]);

    else if (ftpArgv0Is ("XMKD") || ftpArgv0Is ("MKD"))                         return __XMKD__(argv [1]);

    else if (ftpArgv0Is ("XRMD") || ftpArgv0Is ("RMD") || ftpArgv0Is ("DELE"))  return __XRMD__(argv [1]);

    else if (ftpArgv0Is ("CWD"))                                                return __CWD__ (argv [1]);

    else if (ftpArgv0Is ("RNFR"))                                               return __RNFR__ (argv[1]);

    else if (ftpArgv0Is ("RNTO"))                                               return __RNTO__ (argv [1]);

    else if (ftpArgv0Is ("RETR"))                                               return __RETR__ (argv [1]);

    else if (ftpArgv0Is ("STOR"))                                               return __STOR__ (argv [1]);

    return Cstring<300> ("502 command ") + argv [0] + " not implemented\r\n";
}

Cstring<300> ftpServer_t::ftpControlConnection_t::__USER__ (char *userName) {
    __userName__ = userName;
    return "331 enter password\r\n";
}

Cstring<300> ftpServer_t::ftpControlConnection_t::__PASS__ (char *password) {
    if (__getUserHomeDirectory__)
        __homeDirectory__ = __getUserHomeDirectory__ (__userName__, password);
    else
        __homeDirectory__ = "/";

    if (!__fileSystem__.isDirectory (__homeDirectory__))                     
        return "530 invalid user's home directory\r\n";

    if (__homeDirectory__ != "") {
        __workingDirectory__ = __homeDirectory__;

        Cstring<255> s = __homeDirectory__;
        if (s [s.length () - 1] == '/') s [s.length () - 1] = 0;
        if (!s [0]) s = "/";

        getLogQueue () << "[ftpCtrlConn] " << __userName__ << " logged in" << endl;
        return Cstring<300> ("230 logged on, your home directory is \"") + s + "\"\r\n";
    } else {
        getLogQueue () << "[ftpCtrlConn] login denyed for " << __userName__ << endl;
        delay (100);
        return "530 login denyed\r\n";
    }
}

Cstring<300> ftpServer_t::ftpControlConnection_t::__CWD__ (char *directoryName) {
    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";
    if (!__fileSystem__.mounted ())                                                     return "421 file system not mounted\r\n";
    Cstring<255> fullPath = __fileSystem__.makeFullPath (directoryName, __workingDirectory__);
    if (!__fileSystem__.isDirectory (fullPath))                                         return "501 invalid directory name\r\n";
    if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__))    return "550 access denyed\r\n";

    __workingDirectory__ = fullPath;
    return Cstring<300> ("250 your working directory is ") + fullPath + "\r\n";
}

Cstring<300> ftpServer_t::ftpControlConnection_t::__XPWD__ () {
    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";
    if (!__fileSystem__.mounted ())                                                     return "421 file system not mounted\r\n";
    Cstring<255> fullPath = __workingDirectory__;
    if (fullPath [fullPath.length () - 1] == '/')
        fullPath [fullPath.length () - 1] = 0;
    if (!fullPath [0])
        fullPath = "/";

    return Cstring<300> ("257 \"") + fullPath + "\"\r\n";
}

const char *ftpServer_t::ftpControlConnection_t::__XMKD__ (char *directoryName) {
    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";
    if (!__fileSystem__.mounted ())                                                     return "421 file system not mounted\r\n";
    Cstring<255> fullPath = __fileSystem__.makeFullPath (directoryName, __workingDirectory__);
    if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__))    return "550 access denyed\r\n";
    if (!__fileSystem__.mkdir (fullPath))                                               return "550 could not create directory\r\n";

    return "257 directory created\r\n";
}

const char *ftpServer_t::ftpControlConnection_t::__XRMD__ (char *fileOrDirName) {
    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";
    if (!__fileSystem__.mounted ())                                                     return "421 file system not mounted\r\n";
    Cstring<255> fullPath = __fileSystem__.makeFullPath (fileOrDirName, __workingDirectory__);
    if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__))    return "550 access denyed\r\n";
    if (__fileSystem__.isFile (fullPath)) {
        if (__fileSystem__.remove (fullPath))                                           return "250 file deleted\r\n";
                                                                                        return "452 could not delete file\r\n";
    } else {
        if (fullPath == __homeDirectory__)                                              return "550 you can't remove your home directory\r\n";
        if (fullPath == __workingDirectory__)                                           return "550 you can't remove your working directory\r\n";
        if (__fileSystem__.rmdir (fullPath))                                            return "250 directory removed\r\n";
                                                                                        return "452 could not remove directory\r\n";
    }
}

Cstring<300> ftpServer_t::ftpControlConnection_t::__SIZE__ (char *fileName) {
    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";
    if (!__fileSystem__.mounted ())                                                     return "421 file system not mounted\r\n";
    Cstring<255> fullPath = __fileSystem__.makeFullPath (fileName, __workingDirectory__);
    if (!__fileSystem__.userHasRightToAccessFile (fullPath, __homeDirectory__))         return "550 access denyed\r\n";

    unsigned long fSize = 0;
    threadSafeFS::File f = __fileSystem__.open (fullPath, FILE_READ);
    if (f) {
        fSize = f.size ();
        f.close ();
    }

    return Cstring<300> ("213 ") + Cstring<300> (fSize) + "\r\n";
}

void ftpServer_t::ftpControlConnection_t::__closeDataConnection__ () {
    if (__dataConnection__) {
        delete __dataConnection__;
        __dataConnection__ = NULL;
    }
}

// cycle through set of port numbers when FTP server is working in pasive mode
int ftpServer_t::ftpControlConnection_t::__pasiveDataPort__ () {
    static int __lastPasiveDataPort__ = 1024;
    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
        int pasiveDataPort = __lastPasiveDataPort__ = (((__lastPasiveDataPort__ + 1) % 16) + 1024);
    xSemaphoreGive (getLwIpMutex ());
    return pasiveDataPort;
}

// IPv4 PORT command like PORT 10,18,1,26,239,17
const char *ftpServer_t::ftpControlConnection_t::__PORT__ (char *dataConnectionInfo) { 
    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";

    int ip1, ip2, ip3, ip4, p1, p2; // get IP and port that client used to set up data connection server
    char activeServerIP [INET6_ADDRSTRLEN] = "";
    int activeServerPort;
    if (6 == sscanf (dataConnectionInfo, "%i,%i,%i,%i,%i,%i", &ip1, &ip2, &ip3, &ip4, &p1, &p2)) {
        sprintf (activeServerIP, "%i.%i.%i.%i", ip1, ip2, ip3, ip4);
        activeServerPort = p1 << 8 | p2;

        // connect to the FTP client that acts as server now
        __dataConnection__ = new (std::nothrow) tcpClient_t (activeServerIP, activeServerPort);
        if (__dataConnection__ && *__dataConnection__) { // test if connection is created and connected
            __dataConnection__->setIdleTimeout (FTP_DATA_CONNECTION_TIME_OUT);
            return "200 port ok\r\n";
        }
    }
    return "425 can't open active data connection\r\n";
}

// extended IPv6 (and IPv4) EPRT command like EPRT |2|fe80::3d98:8793:4cf0:f618|61166|
const char *ftpServer_t::ftpControlConnection_t::__EPRT__ (char *dataConnectionInfo) {
    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";

    char activeServerIP [INET6_ADDRSTRLEN];
    int activeServerPort;

    if (2 == sscanf (dataConnectionInfo, "|%*d|%39[^|]|%d|", activeServerIP, &activeServerPort)) {
        // connect to the FTP client that act as server now
        __dataConnection__ = new (std::nothrow) tcpClient_t (activeServerIP, activeServerPort);
        if (__dataConnection__ && *__dataConnection__) { // test if connection is created and connected
            __dataConnection__->setIdleTimeout (FTP_DATA_CONNECTION_TIME_OUT);
            return "200 port ok\r\n";
        }
    }
    return "425 can't open active data connection\r\n";
}

// IPv4 PASV command
const char *ftpServer_t::ftpControlConnection_t::__PASV__ () {
    __closeDataConnection__ ();

    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";

    int ip1, ip2, ip3, ip4, p1, p2; // get FTP server IP and next free port
    if (4 != sscanf (getServerIP (), "%i.%i.%i.%i", &ip1, &ip2, &ip3, &ip4)) {
        getLogQueue () << "[ftpCtrlConn] can't parse server IP: " << getServerIP () << endl;
        return "425 can't open passive data connection\r\n";
    }

    // get next free port
    int pasiveDataPort = __pasiveDataPort__ ();
    p2 = pasiveDataPort % 256;
    p1 = pasiveDataPort / 256;

    // set up a new server for data connection and send connection information to the FTP client so it can connect to it
    tcpServer_t passiveDataServer (pasiveDataPort, NULL, false);
    if (passiveDataServer) { // if the server is running
        // notify FTP client about data connection IP and port
        Cstring<300> s;
        sprintf (s, "227 entering passive mode (%i,%i,%i,%i,%i,%i)\r\n", ip1, ip2, ip3, ip4, p1, p2);
        if (sendString (s) <= 0)
            return "";

        // wait for data connection
        __dataConnection__ = NULL;
        unsigned long startMillis = millis ();
        while (__dataConnection__ == NULL && millis () - startMillis < FTP_DATA_CONNECTION_TIME_OUT * 1000) {
            delay (25);
            __dataConnection__ = passiveDataServer.accept ();
        }

        if (__dataConnection__) {
            __dataConnection__->setIdleTimeout (FTP_DATA_CONNECTION_TIME_OUT);
            return "";
        } else {
            delete __dataConnection__;
            __dataConnection__ = NULL;
            return "425 can't open passive data connection\r\n";
        }
    }
    return "425 can't open passive data connection\r\n";
}

// extended IPv6 (and IPv4) EPSV command
const char *ftpServer_t::ftpControlConnection_t::__EPSV__ () {
    __closeDataConnection__ ();

    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";

    // get next free port
    int pasiveDataPort = __pasiveDataPort__ ();

    // set up a new server for data connection and send connection information to the FTP client so it can connect to it
    tcpServer_t passiveDataServer (pasiveDataPort, NULL, false);
    if (passiveDataServer) { // if the server is running
        // notify FTP client about data connection IP and port
        Cstring<300> s;
        sprintf (s, "229 entering passive mode (|||%i|)\r\n", pasiveDataPort);
        if (sendString (s) <= 0)
            return "";

        // wait for data connection
        __dataConnection__ = NULL;
        unsigned long startMillis = millis ();
        while (__dataConnection__ == NULL && millis () - startMillis < FTP_DATA_CONNECTION_TIME_OUT * 1000) {
            delay (25);
            __dataConnection__ = passiveDataServer.accept ();
        }

        if (__dataConnection__) {
            delete __dataConnection__;
            __dataConnection__ = NULL;
            return "425 can't set passive data connection timeout\r\n";
        }

        __dataConnection__->setIdleTimeout (FTP_DATA_CONNECTION_TIME_OUT);
        return "";
    }
    return "425 can't open passive data connection\r\n";
}

// NLST is preceeded by PORT or PASV so data connection should already be opened
const char *ftpServer_t::ftpControlConnection_t::__NLST__ (char *directoryName) {
    const char *retVal = "";
    if (__homeDirectory__ == "") {
        retVal = "530 not logged in\r\n";
    } else {
        if (!__fileSystem__.mounted ()) {
            retVal = "421 file system not mounted\r\n";
        } else {
            Cstring<255> fullPath = __fileSystem__.makeFullPath (directoryName, __workingDirectory__);
            if (fullPath == "" || !__fileSystem__.isDirectory (fullPath)) {
                retVal = "501 invalid directory name\r\n";
            } else {
                if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__)) {
                    retVal = "550 access denyed\r\n";
                } else {
                    if (sendString ("150 starting data transfer\r\n") > 0) {
                        threadSafeFS::File d = __fileSystem__.open (fullPath);
                        if (d) {
                            for (threadSafeFS::File f = d.openNextFile (); f; f = d.openNextFile ()) {
                                Cstring<255> fullFileName = fullPath;
                                if (fullFileName [fullFileName.length () - 1] != '/')
                                    fullFileName += '/';
                                fullFileName += f.name ();
                                if (__dataConnection__->sendString (__fileSystem__.fileInformation (fullFileName) + "\r\n") <= 0) {
                                    retVal = "426 data transfer error\r\n";
                                    break;
                                }                                            
                            }                                    
                            d.close ();
                        }       
                        if (!*retVal)
                            sendString ("226 data transfer complete\r\n");
                    }
                }
            }
        }
    }

    __closeDataConnection__ ();
    return retVal;
}

const char *ftpServer_t::ftpControlConnection_t::__RNFR__ (char *fileOrDirName) {
    __rnfrIs__ = ' ';
    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";
    if (!__fileSystem__.mounted ())                                                     return "421 file system not mounted\r\n";
    Cstring<255> fullPath =
        __fileSystem__.makeFullPath (fileOrDirName, __workingDirectory__);
    if (fullPath == "")                                                                 return "501 invalid file or directory name\r\n";
    if (__fileSystem__.isDirectory (fullPath)) {
        if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__)) return "550 access denyed\r\n";
        __rnfrIs__ = 'd';
    } else if (__fileSystem__.isFile (fullPath)) {
        if (!__fileSystem__.userHasRightToAccessFile (fullPath, __homeDirectory__))
            return "550 access denyed\r\n";
        __rnfrIs__ = 'f';
    } else {
        return "501 invalid file or directory name\r\n";
    }
    __rnfrPath__ = fullPath;

    return "350 need more information\r\n";
}

const char *ftpServer_t::ftpControlConnection_t::__RNTO__ (char *fileOrDirName) {
    if (__homeDirectory__ == "")                                                        return "530 not logged in\r\n";
    if (!__fileSystem__.mounted ())                                                     return "421 file system not mounted\r\n";
    Cstring<255> fullPath = __fileSystem__.makeFullPath (fileOrDirName, __workingDirectory__);
    if (fullPath == "")                                                                 return "501 invalid file or directory name\r\n";
    if (__rnfrIs__ == 'd') {
        if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__)) return "550 access denyed\r\n";
    } else if (__rnfrIs__ == 'f') {
        if (!__fileSystem__.userHasRightToAccessFile (fullPath, __homeDirectory__))     return "550 access denyed\r\n";
    } else {
        return "501 invalid file or directory name\r\n";
    }

    if (__fileSystem__.rename (__rnfrPath__, fullPath))
        return "250 renamed\r\n";
    return "553 unable to rename\r\n";
}

const char *ftpServer_t::ftpControlConnection_t::__RETR__ (char *fileName) {
    const char *retVal = "";
    if (__homeDirectory__ == "") {
        retVal = "530 not logged in\r\n";
    } else {
        if (!__fileSystem__.mounted ()) {
            retVal = "421 file system not mounted\r\n";
        } else {
            Cstring<255> fullPath = __fileSystem__.makeFullPath (fileName, __workingDirectory__);
            if (fullPath == "" || __fileSystem__.isDirectory (fullPath)) {
                retVal = "501 invalid file name\r\n";
            } else {
                if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__)) {
                    retVal = "550 access denyed\r\n";
                } else {
                    if (sendString ("150 starting data transfer\r\n") > 0) {
                        int bytesReadTotal = 0;
                        int bytesSentTotal = 0;
                        threadSafeFS::File f = __fileSystem__.open (fullPath, FILE_READ);
                        if (f) {
                            char buff [1024];
                            do {
                                int bytesReadThisTime =
                                    f.read ((uint8_t *) buff, sizeof (buff));
                                if (bytesReadThisTime == 0)
                                    break;
                                bytesReadTotal += bytesReadThisTime;
                                int bytesSentThisTime =
                                    __dataConnection__->sendBlock (buff, bytesReadThisTime);
                                if (bytesSentThisTime != bytesReadThisTime) {
                                    retVal = "426 data transfer error\r\n";
                                    break;
                                }
                                bytesSentTotal += bytesSentThisTime;
                            } while (true);
                            f.close ();
                        } else {
                            retVal = "450 can not open the file\r\n";
                        }

                        if (!*retVal)
                            sendString ("226 data transfer complete\r\n");
                    }
                }
            }
        }
    }

    __closeDataConnection__ ();
    return retVal;
}

const char *ftpServer_t::ftpControlConnection_t::__STOR__ (char *fileName) {
    const char *retVal = "";
    if (__homeDirectory__ == "") {
        retVal = "530 not logged in\r\n";
    } else {
        if (!__fileSystem__.mounted ()) {
            retVal = "421 file system not mounted\r\n";
        } else {
            Cstring<255> fullPath =
                __fileSystem__.makeFullPath (fileName, __workingDirectory__);
            if (fullPath == "" || __fileSystem__.isDirectory (fullPath)) {
                retVal = "501 invalid file name\r\n";
            } else {
                if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__)) {
                    retVal = "550 access denyed\r\n";
                } else {
                    if (sendString ("150 starting data transfer\r\n") > 0) {
                        int bytesRecvTotal = 0;
                        int bytesWrittenTotal = 0;
                        threadSafeFS::File f = __fileSystem__.open (fullPath, FILE_WRITE);
                        if (f) {
                            char buff [1024];
                            do {
                                int bytesRecvThisTime = __dataConnection__->recv (buff, sizeof (buff));
                                if (bytesRecvThisTime < 0) {
                                    retVal = "426 data transfer error\r\n";
                                    break;
                                }
                                if (bytesRecvThisTime == 0)
                                    break;
                                bytesRecvTotal += bytesRecvThisTime;
                                int bytesWrittenThisTime = f.write ((uint8_t *) buff, bytesRecvThisTime);
                                bytesWrittenTotal += bytesWrittenThisTime;
                                if (bytesWrittenThisTime != bytesRecvThisTime) {
                                    retVal = "450 can not write the file\r\n";
                                    break;
                                }
                            } while (true);
                            f.close ();
                        } else {
                            retVal = "450 can not open the file\r\n";
                        }

                        if (!*retVal)
                            sendString ("226 data transfer complete\r\n");
                    }
                }
            }
        }
    }

    __closeDataConnection__ ();
    return retVal;
}

// ----- ftpServer_t implementation -----

ftpServer_t::ftpServer_t (threadSafeFS::FS fileSystem,
                          Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName,const Cstring<64>& password),
                          int serverPort,
                          bool (*firewallCallback) (char *clientIP, char *serverIP),
                          bool runListenerInItsOwnTask) : tcpServer_t (serverPort, firewallCallback, runListenerInItsOwnTask),
                                                          __fileSystem__ (fileSystem),
                                                          __getUserHomeDirectory__ (getUserHomeDirectory) {
}

tcpConnection_t *ftpServer_t::__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) {
    #define ftpServiceUnavailableReply "421 FTP service is currently unavailable. Free heap: %u bytes. Free heap in one piece: %u bytes.\r\n"

    ftpControlConnection_t *connection = new (std::nothrow) ftpControlConnection_t (__fileSystem__,
                                                                                    __getUserHomeDirectory__,
                                                                                    connectionSocket,
                                                                                    clientIP,
                                                                                    serverIP);

    if (!connection) {
        getLogQueue () << "[ftpServer] " << "can't create connection instance, out of memory" << endl;
        char s [128];
        sprintf (s, ftpServiceUnavailableReply, esp_get_free_heap_size (), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT));
        send (connectionSocket, s, strlen (s), 0);
        close (connectionSocket);
        return NULL;
    }

    connection->setIdleTimeout (FTP_CONTROL_CONNECTION_TIME_OUT);

    #define tskNORMAL_PRIORITY (tskIDLE_PRIORITY + 1)
    if (pdPASS != xTaskCreate ([] (void *thisInstance) {
                                                            ftpControlConnection_t *ths = (ftpControlConnection_t *) thisInstance;
                                                            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                                                __runningTcpConnections__++;
                                                            xSemaphoreGive (getLwIpMutex ());

                                                            ths->__runConnectionTask__ ();

                                                            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                                                __runningTcpConnections__--;
                                                            xSemaphoreGive (getLwIpMutex ());

                                                            delete ths;
                                                            vTaskDelete (NULL);
                                                        }, "ftpCtrlConn", FTP_CONTROL_CONNECTION_STACK_SIZE, connection, tskNORMAL_PRIORITY, NULL)) {
        getLogQueue () << "[ftpServer] " << "can't create connection task, out of memory" << endl;
        char s [128];
        sprintf (s, ftpServiceUnavailableReply, esp_get_free_heap_size (), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT));
        connection->sendString (s);
        delete connection;
        return NULL;
    }

    return NULL;
}
