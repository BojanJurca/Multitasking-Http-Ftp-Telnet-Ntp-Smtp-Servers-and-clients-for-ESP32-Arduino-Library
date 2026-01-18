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


#pragma once
#ifndef __FTP_SERVER__
    #define __FTP_SERVER__

    #include <WiFi.h>
    #include <LwIpMutex.h>
    #include "tcpServer.h"
    #include "tcpConnection.h"
    #include "tcpClient.h"
    #include <Cstring.hpp>      // include LightweightSTL library: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
    #include <FS.h>
    #include <threadSafeFS.h>
    #ifdef __DMESG__
        #include <dmesg.hpp>      // use dmesg if #included
        #define endl ""
    #else
        #include <ostream.hpp>    // use serial console if not
    #endif


    // TUNING PARAMETERS

    #ifndef FTP_CONTROL_CONNECTION_STACK_SIZE
        #define FTP_CONTROL_CONNECTION_STACK_SIZE (6 * 1024)    // a good first estimate how to set this parameter would be to always leave at least 1 KB of each ftpControlConnection stack unused
    #endif
    #ifndef FTP_CMDLINE_BUFFER_SIZE
        #define FTP_CMDLINE_BUFFER_SIZE 300                     // reading and temporary keeping FTP command lines                    
    #endif
    #ifndef FTP_SESSION_MAX_ARGC
        #define FTP_SESSION_MAX_ARGC 5                          // max number of arguments in command line, 5 is enough for FTP protocol                       
    #endif
    #ifndef FTP_CONTROL_CONNECTION_TIME_OUT
        #define FTP_CONTROL_CONNECTION_TIME_OUT 300             // 300 s = 5 min, set to 0 for infinite            
    #endif
    #ifndef FTP_DATA_CONNECTION_TIME_OUT
        #define FTP_DATA_CONNECTION_TIME_OUT 3                  // 3 s, set to 0 for infinite            
    #endif

    #ifndef HOSTNAME
        #define HOSTNAME "Esp32Server"                          // use default HOSTNAME if not defined previously
    #endif

    class ftpServer_t : public tcpServer_t {

        public:

            class ftpControlConnection_t : public tcpConnection_t {

                friend class ftpServer_t;

            private:

                threadSafeFS::FS __fileSystem__;
                Cstring<255> (*__getUserHomeDirectory__) (const Cstring<64>& userName, const Cstring<64>& password) = NULL;

                // FTP session related variables
                char __cmdLine__ [FTP_CMDLINE_BUFFER_SIZE];

                Cstring<64>  __userName__;
                Cstring<255> __homeDirectory__;
                Cstring<255> __workingDirectory__;

                static UBaseType_t __lastHighWaterMark__;

                tcpClient_t      *__activeDataClient__  = NULL;
                tcpConnection_t  *__dataConnection__    = NULL;

                Cstring<255> __rnfrPath__;
                char __rnfrIs__; // 'f' or 'd'

                #ifdef __DMESG__
                    inline auto& getLogQueue () __attribute__((always_inline)) { return dmesgQueue; } // use dmesg if #included
                    #define endl ""
                #else
                    inline auto& getLogQueue () __attribute__((always_inline)) { return cout; } // use serial console if not
                #endif        

            public:

                ftpControlConnection_t (threadSafeFS::FS fileSystem,
                                        Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password),
                                        int connectionSocket,
                                        char *clientIP,
                                        char *serverIP);

                ~ftpControlConnection_t ();

                // FTP session related variables
                inline char *getUserName () __attribute__((always_inline)) { return __userName__; }
                inline char *getHomeDirectory () __attribute__((always_inline)) { return __homeDirectory__; }
                inline char *getWorkingDirectory () __attribute__((always_inline)) { return __workingDirectory__; }

            private:

                // core task logic
                void __runConnectionTask__ ();

                // command dispatcher
                cstring __internalCommandHandler__ (int argc, char *argv []);

                // user/session commands
                Cstring<300>   __USER__ (char *userName);
                Cstring<300>   __PASS__ (char *password);
                Cstring<300>   __CWD__ (char *directoryName);
                Cstring<300>   __XPWD__ ();
                const char    *__XMKD__ (char *directoryName);
                const char    *__XRMD__ (char *fileOrDirName);
                Cstring<300>   __SIZE__ (char *fileName);

                // data connection management
                void           __closeDataConnection__ ();
                int            __pasiveDataPort__ ();
                const char    *__PORT__ (char *dataConnectionInfo);
                const char    *__EPRT__ (char *dataConnectionInfo);
                const char    *__PASV__ ();
                const char    *__EPSV__ ();
                const char    *__NLST__ (char *directoryName);
                const char    *__RNFR__ (char *fileOrDirName);
                const char    *__RNTO__ (char *fileOrDirName);
                const char    *__RETR__ (char *fileName);
                const char    *__STOR__ (char *fileName);
            };

        private:

            threadSafeFS::FS __fileSystem__;
            Cstring<255> (*__getUserHomeDirectory__) (const Cstring<64>& userName, const Cstring<64>& password) = NULL;

            #ifdef __DMESG__
                inline auto& getLogQueue () __attribute__((always_inline)) { return dmesgQueue; } // use dmesg if #included
                #define endl ""
            #else
                inline auto& getLogQueue () __attribute__((always_inline)) { return cout; } // use serial console if not
            #endif        

        public:

            ftpServer_t (threadSafeFS::FS fileSystem,
                         Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL,
                         int serverPort = 21,
                         bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                         bool runListenerInItsOwnTask = true);

            tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) override;

            // accept any connection, the client will get notified in __createConnectionInstance__
            inline tcpConnection_t *accept () __attribute__((always_inline)) { return tcpServer_t::accept (); }
    };

#endif
