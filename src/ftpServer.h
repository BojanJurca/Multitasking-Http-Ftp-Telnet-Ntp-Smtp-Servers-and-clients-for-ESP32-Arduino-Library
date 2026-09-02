/*

    ftpServer.h 
  
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  

    Aug 12, 2026, Bojan Jurca


    Multitasking/thread-safe classes and functions: 

        inherits from:  ─────────▶                                                                                          ┌───────────────────┐
        uses:           •••••••••▶                                                                                           │ ntpClient_t       │
                                                                                                                             └───────────────────┘
                                                                                                                             ┌───────────────────┐
                                                                                        ••••••••••••••••••••••••••••••••••••▶│ httpsClient       │
                                                                                        •                                    └───────────────────┘
                                                                                        •                                    ┌───────────────────┐
                                                                                        •                           ••••••••▶│ httpClient        │
                                                                                        •                           •        └───────────────────┘
                                                                                        •                           •        ┌───────────────────┐
                                                                                        •                           ••••••••▶│ smtpClient        │
                                                                                        •                           •        └───────────────────┘
                                                                                        •                           •                                             
                                                                                        •                           •                             
┌──────────────────────┐                                                                •                  ┌────────•──────────┐                  
│ tcpServer_t          │••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••▶│ tcpConnection_t   │                  
└──────────────────────┘                                    •                           •               •  └───────────────────┘                  
           ▲                                                •                           •               •            ▲                            
           │                                                •                           •               •            │                            
           │  ┌─────────────────┐                           •                ┌──────────────────────┐   •            │                            
           │──│ httpServer_t    │ ••••••••••••••••••••••••••••••••••••••••••▶│ tlsConnection_t      │────────────────┘                            
           │  └─────────────────┘                           •                └──────────────────────┘   •            │                            
           │           ▲                                    •                           ▲               •            │                            
           │           │                                    •                           •               •            │                            
           │           │                                    •           ┌───────────────────────────┐   •            │                            
           │  ┌──────────────────┐                          •           │ httpServer_t::webSocket_t │••••            │                            
           │  │ httpsServer_t    │                          •           └───────────────────────────┘                │                            
           │  └──────────────────┘                          •                           ▲                            │                            
           │                                                •                           │                            │                            
           │                                                •           ┌────────────────────────────────┐           │                            
           │                                                •           │ httpServer_t::httpConnection_t │           │                            
           │                                                •           └────────────────────────────────┘           │                            
           │                                                •                                                        │                            
           │  ┌──────────────────┐                          •           ┌────────────────────────────────────┐       │                            
           │──│ ftpServer_t      │•••••••••••••••••••••••••••••••••••••▶│ftpServer_t::ftpControlConnection_t │──────│                            
           │  └──────────────────┘                                      └────────────────────────────────────┘       │                            
           │                                                                                                         │                                                   
           │  ┌──────────────────┐                                      ┌───────────────────────────────────┐        │                            
           └──│ telnetServer_t   │•••••••••••••••••••••••••••••••••••••▶│telnetServer_t::telnetConnection_t │───────┘
              └──────────────────┘                                      └───────────────────────────────────┘                                    


Edit/view: https://cascii.app/e83d5                           

*/


#pragma once
#ifndef __FTP_SERVER_H__
    #define __FTP_SERVER_H__


    #include "tcpServer.h"
    #include <Cstring.hpp>      // include LightweightSTL library: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
    #include <FS.h>
    #include <threadSafeFS.h>


    // TUNING PARAMETERS

    #define FTP_CONTROL_CONNECTION_STACK_SIZE (8 * 1024)        // a good first estimate how to set this parameter would be to always leave at least 1 KB of each ftpControlConnection stack unused
    #define FTP_CMDLINE_BUFFER_SIZE 300                         // reading and temporary keeping FTP command lines                    
    #define FTP_SESSION_MAX_ARGC 5                              // max number of arguments in command line, 5 is enough for FTP protocol                       
    #define FTP_CONTROL_CONNECTION_TIME_OUT 300                 // 300 s = 5 min, set to 0 for infinite            
    #define FTP_DATA_CONNECTION_TIME_OUT 3                      // 3 s, set to 0 for infinite            

    #ifndef HOSTNAME
        #define HOSTNAME "Esp32Server"                          // use default HOSTNAME if not defined previously
    #endif


    class ftpServer_t : public tcpServer_t {

        public:

            class ftpControlConnection_t : public tcpConnection_t {

                friend class ftpServer_t;

            private:

                threadSafeFS::FS& __fileSystem__;
                Cstring<255> (*__getUserHomeDirectory__) (const Cstring<64>& userName, const Cstring<64>& password) = NULL;

                // FTP session related variables
                char __cmdLine__ [FTP_CMDLINE_BUFFER_SIZE];

                Cstring<64>  __userName__;
                Cstring<255> __homeDirectory__;
                Cstring<255> __workingDirectory__;

                static UBaseType_t __lastHighWaterMark__;

                //tcpConnection_t  *__activeDataClient__  = NULL;
                tcpConnection_t  *__dataConnection__    = NULL;

                Cstring<255> __rnfrPath__;
                char __rnfrIs__; // 'f' or 'd'

            public:

                ftpControlConnection_t (threadSafeFS::FS& fileSystem,
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

            threadSafeFS::FS& __fileSystem__;
            Cstring<255> (*__getUserHomeDirectory__) (const Cstring<64>& userName, const Cstring<64>& password) = NULL;

        public:

            ftpServer_t (threadSafeFS::FS& fileSystem,
                         Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL,
                         int serverPort = 21,
                         bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                         bool runListenerInItsOwnTask = true);

            #if TSFS_FS_COUNT == 1 // there is only one file system wrapped ...
                ftpServer_t (Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL,
                             int serverPort = 21,
                             bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                             bool runListenerInItsOwnTask = true);
            #endif

            tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) override;

            // accept any connection, the client will get notified in __createConnectionInstance__
            inline tcpConnection_t *accept () __attribute__((always_inline)) { return tcpServer_t::accept (); }
    };


    // ----- ftpServer_t implementation -----

    #if TSFS_FS_COUNT == 1 // there is only one file system wrapped ...
        ftpServer_t::ftpServer_t (Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password),
                                  int serverPort,
                                  bool (*firewallCallback) (char *clientIP, char *serverIP),
                                  bool runListenerInItsOwnTask) : tcpServer_t (serverPort, firewallCallback, runListenerInItsOwnTask),
                                                                  __fileSystem__ (tsfs), // ... use the only file system wrapper
                                                                  __getUserHomeDirectory__ (getUserHomeDirectory) {
        }
    #endif

#endif
