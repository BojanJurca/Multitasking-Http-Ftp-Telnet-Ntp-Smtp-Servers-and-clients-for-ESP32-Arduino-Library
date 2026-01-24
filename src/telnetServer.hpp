/*

    telnetServer.hpp

    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


    January 1, 2026, Bojan Jurca


    Classes implemented/used in this module:

        telnetServer_t
        telnetServer_t::telnetConnection_t

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

                  There is normally only one TCP connection per telnet session. These terms are pretty much interchangeable when we are talking about telnet.

     - "session" applies to user interaction between login and logout

                  The first thing that the user does when a TCP connection is established is logging in and the last thing is logging out. If TCP
                  connection drops for some reason the user is automatically logged out.

      - "buffer size" is the number of bytes that can be placed in a buffer. 
      
                  In case of C 0-terminated strings the terminating 0 character should be included in "buffer size".

    Telnet protocol is basically a straightforward non synchronized transfer of text data in both directions, from client to server and vice versa.
    This implementation also adds processing command lines by detecting the end-of-line, parsing command line to arguments and then executing
    some already built-in commands and sending replies back to the client. In addition the calling program can provide its own command handler to handle
    some commands itself. Special telnet commands, so called IAC ("Interpret As Command") commands are placed within text stream and should be
    extracted from it and processed by both, the server and the client. All IAC commands start with character 255 (IAC character) and are followed
    by other characters. The number of characters following IAC characters varies from command to command. For example: the server may request
    the client to report its window size by sending IAC (255) DO (253) NAWS (31) and then the client replies with IAC (255) SB (250) NAWS (31) following
    with 2 bytes of window width and 2 bytes of window height and then IAC (255) SE (240).

*/


#pragma once
#ifndef __TELNET_SERVER__
    #define __TELNET_SERVER__


    #include <esp_task_wdt.h>
    #include <esp_wifi.h>
    #include <Cstring.hpp>      // include LightweightSTL library: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
    #include "tcpServer.h"
    #include "tcpConnection.h"
    #include "tcpClient.h"

    #ifdef __THREAD_SAFE_FS__
        #include <FS.h>
    #endif


    // INCLUDE/EXCLUDE TELNET COMMANDS

    #ifndef TELNET_CLEAR_COMMAND
        #define TELNET_UNAME_COMMAND 1      // 0=exclude, 1=include, uname included by default
    #endif
    #ifndef TELNET_UNAME_COMMAND
        #define TELNET_UNAME_COMMAND 1      // 0=exclude, 1=include, uname included by default
    #endif
    #ifndef TELNET_FREE_COMMAND
        #define TELNET_FREE_COMMAND 1       // 0=exclude, 1=include, free included by default
    #endif
    #ifndef TELNET_NOHUP_COMMAND
        #define TELNET_NOHUP_COMMAND 1      // 0=exclude, 1=include, nohup included by default
    #endif
    #ifndef TELNET_REBOOT_COMMAND
        #define TELNET_REBOOT_COMMAND 1     // 0=exclude, 1=include, reboot included by default
    #endif
    #ifndef TELNET_DMESG_COMMAND
        #define TELNET_DMESG_COMMAND 1      // 0=exclude, 1=include, dmesg included by default
    #endif
    #ifndef TELNET_UPTIME_COMMAND
        #define TELNET_UPTIME_COMMAND 1     // 0=exclude, 1=include, date included by default
    #endif
    #ifndef TELNET_DATE_COMMAND
        #define TELNET_DATE_COMMAND 1       // 0=exclude, 1=include, date included by default
    #endif
    #ifndef TELNET_NTPDATE_COMMAND
        #define TELNET_NTPDATE_COMMAND 1    // 0=exclude, 1=include, ntpdate included by default
    #endif
    #ifndef TELNET_CRONTAB_COMMAND
        #define TELNET_CRONTAB_COMMAND 1    // 0=exclude, 1=include, crontab included by default
    #endif
    #ifndef TELNET_PING_COMMAND
        #define TELNET_PING_COMMAND 1       // 0=exclude, 1=include, ping included by default
    #endif
    #ifndef TELNET_IFCONFIG_COMMAND
        #define TELNET_IFCONFIG_COMMAND 1   // 0=exclude, 1=include, ifconfig included by default
    #endif
    #ifndef TELNET_IW_COMMAND
        #define TELNET_IW_COMMAND 1         // 0=exclude, 1=include, iw included by default
    #endif
    #ifndef TELNET_NETSTAT_COMMAND
        #define TELNET_NETSTAT_COMMAND 1    // 0=exclude, 1=include, netstat included by default
    #endif
    #ifndef TELNET_KILL_COMMAND
        #define TELNET_KILL_COMMAND 1       // 0=exclude, 1=include, kill included by default
    #endif
    #ifndef TELNET_CURL_COMMAND
        #define TELNET_CURL_COMMAND 1       // 0=exclude, 1=include, curl included by default
    #endif
    #ifndef TELNET_SENDMAIL_COMMAND
        #define TELNET_SENDMAIL_COMMAND 1   // 0=exclude, 1=include, sendmail included by default
    #endif
    #ifndef TELNET_LS_COMMAND
        #define TELNET_LS_COMMAND 1         // 0=exclude, 1=include, ls included by default
    #endif
    #ifndef TELNET_TREE_COMMAND
        #define TELNET_TREE_COMMAND 1       // 0=exclude, 1=include, tree included by default
    #endif
    #ifndef TELNET_MKDIR_COMMAND
        #define TELNET_MKDIR_COMMAND 1      // 0=exclude, 1=include, mkdir included by default
    #endif
    #ifndef TELNET_RMDIR_COMMAND
        #define TELNET_RMDIR_COMMAND 1      // 0=exclude, 1=include, rmdir included by default
    #endif
    #ifndef TELNET_CD_COMMAND
        #define TELNET_CD_COMMAND 1         // 0=exclude, 1=include, cd included by default
    #endif
    #ifndef TELNET_PWD_COMMAND
        #define TELNET_PWD_COMMAND 1        // 0=exclude, 1=include, pwd included by default
    #endif
    #ifndef TELNET_CAT_COMMAND
        #define TELNET_CAT_COMMAND 1        // 0=exclude, 1=include, cat included by default
    #endif
    #ifndef TELNET_VI_COMMAND
        #define TELNET_VI_COMMAND 1         // 0=exclude, 1=include, vi included by default
    #endif
    #ifndef TELNET_CP_COMMAND
        #define TELNET_CP_COMMAND 1         // 0=exclude, 1=include, cp included by default
    #endif
    #ifndef TELNET_RM_COMMAND
        #define TELNET_RM_COMMAND 1         // 0=exclude, 1=include, rm included by default
    #endif



    #ifndef TELNET_USERADD_COMMAND
        #define TELNET_USERADD_COMMAND 1    // 0=exclude, 1 = include, useradd included by default
    #endif
    #ifndef TELNET_USERDEL_COMMAND
        #define TELNET_USERDEL_COMMAND 1    // 0=exclude, 1 = include, userdel included by default
    #endif
    #ifndef TELNET_PASSWD_COMMAND
        #define TELNET_PASSWD_COMMAND 1     // 0=exclude, 1 = include, userdel included by default
    #endif


    #ifdef __DMESG__ 
        #include <dmesg.hpp>
        #define endl ""
    #else
        #include <ostream.hpp>
        #define dmesgQueue cout<<endl       // if dmesg.hpp is not included just output the messages to the serial console
    #endif

    #if TELNET_UPTIME_COMMAND == 1 or TELNET_DATE_COMMAND == 1 or TELNET_NTPDATE_COMMAND == 1
        #include <ntpClient.h>
    #endif

    #if TELNET_PING_COMMAND == 1
        #include <ThreadSafePing.h>
    #endif
    #if TELNET_CURL_COMMAND == 1 
        #include <httpClient.h>
    #endif
    #if TELNET_SENDMAIL_COMMAND == 1 
        #include <smtpClient.h>
    #endif
    #ifdef TELNET_TREE_COMMAND == 1
        #include <queue.hpp>   	            // for tree command only, // include LightweightSTL library: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
    #endif
    #ifdef TELNET_VI_COMMAND == 1
        #include <vector.hpp>               // for vi command only, include LightweightSTL library: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
    #endif


    // TUNNING PARAMETERS

    #ifndef TELNET_CONNECTION_STACK_SIZE
        #define TELNET_CONNECTION_STACK_SIZE (9 * 1024)             // a good first estimate how to set this parameter would be to always leave at least 1 KB of each telnetConnection stack unused
    #endif
    #ifndef TELNET_CONNECTION_TIME_OUT
        #define TELNET_CONNECTION_TIME_OUT 256                      // 300 s = 5 min, 0 for infinite
    #endif
    #ifndef TELNET_CMDLINE_BUFFER_SIZE
        #define TELNET_CMDLINE_BUFFER_SIZE 300                      // reading and temporary keeping telnet command lines
    #endif
    #ifndef TELNET_SESSION_MAX_ARGC
        #define TELNET_SESSION_MAX_ARGC 24                          // max number of arguments in command line
    #endif
    // #define SWITHC_DEL_AND_BACKSPACE                            // uncomment this line to swithc the meaning of these keys - this would be suitable for Putty and Linux Telnet clients

    #ifndef HOSTNAME
        #define HOSTNAME "Esp32Server"                                 // use default if not defined previously
    #endif
    #if CONFIG_IDF_TARGET_ESP32
        #define MACHINETYPE             "ESP32"   
    #elif CONFIG_IDF_TARGET_ESP32S2
        #define MACHINETYPE             "ESP32-S2"    
    #elif CONFIG_IDF_TARGET_ESP32S3
        #define MACHINETYPE             "ESP32-S3"
    #elif CONFIG_IDF_TARGET_ESP32C3
        #define MACHINETYPE             "ESP32-C3"        
    #elif CONFIG_IDF_TARGET_ESP32C6
        #define MACHINETYPE             "ESP32-C6"
    #elif CONFIG_IDF_TARGET_ESP32H2
        #define MACHINETYPE             "ESP32-H2"
    #else
        #define MACHINETYPE             "ESP32 (other)"
    #endif


    // ----- CODE -----


    // define some IAC telnet commands
    #define __IAC__               0Xff   // 255 as number
    #define IAC                   "\xff" // 255 as string
    #define __DONT__              0xfe   // 254 as number
    #define DONT                  "\xfe" // 254 as string
    #define __DO__                0xfd   // 253 as number
    #define DO                    "\xfd" // 253 as string
    #define __WONT__              0xfc   // 252 as number
    #define WONT                  "\xfc" // 252 as string
    #define __WILL__              0xfb   // 251 as number
    #define WILL                  "\xfb" // 251 as string
    #define __SB__                0xfa   // 250 as number
    #define SB                    "\xfa" // 250 as string
    #define __SE__                0xf0   // 240 as number
    #define SE                    "\xf0" // 240 as string
    #define __CHARSET__           0x2A   //   42 as number
    #define CHARSET               "\x2A" //   42 as string 
    #define __LINEMODE__          0x22   //  34 as number
    #define LINEMODE              "\x22" //  34 as string
    #define __NAWS__              0x1f   //  31 as number
    #define NAWS                  "\x1f" //  31 as string
    #define __SUPPRESS_GO_AHEAD__ 0x03   //   3 as number
    #define SUPPRESS_GO_AHEAD     "\x03" //   3 as string
    #define __RESPONSE__          0x02   //   2 as number
    #define RESPONSE              "\x02" //   2 as string 
    #define __ECHO__              0x01   //   1 as number
    #define ECHO                  "\x01" //   1 as string 
    #define __REQUEST__           0x01   //   1 as number
    #define REQUEST               "\x01" //   1 as string 

    // IAC negotiotion used here

    // server -> client: IAC WILL ECHO 
    // client -> server: IAC DO ECHO

    // server -> client: IAC WILL SUPPRESS_GO_AHEAD 
    // client -> server: IAC DO SUPPRESS_GO_AHEAD
    
    // server -> clint: IAC DO NAWS
    // client -> server: IAC SB NAWS windowWidht windowHeight SE

    // most of the Telnet clients do not support charset so we are skipping this for now
    // servet -> client: IAC WILL CHARSET
    // server <- client: IAC DO CHARSET
    // server -> client: IAC SB CHARSET REQUEST UTF-8 IAC SE
    // client <- server: IAC SB CHARSET RESPONSE UTF-8 IAC SE (hopefully, but won't check)


    class telnetServer_t : public tcpServer_t {

        public:

            class telnetConnection_t : public tcpConnection_t {

                friend class telnetServer_t;

                private:

                    #ifdef __THREAD_SAFE_FS__
                        threadSafeFS::FS __fileSystem__;
                    #endif
                    Cstring<255> (*__getUserHomeDirectory__) (const Cstring<64>& userName, const Cstring<64>& password) = NULL;
                    String (*__telnetCommandHandlerCallback__) (int argc, char *argv [], telnetConnection_t *tcn);  // will be initialized in constructor

                    // telnet session related variables
                    Cstring<64> __userName__;           // empty ba default
                    Cstring<255> __homeDirectory__;     // empty by default
                    Cstring<255> __workingDirectory__;  // empty by default
                    unsigned char __peekedChar__ = 0;
                    char __cmdLine__ [TELNET_CMDLINE_BUFFER_SIZE]; // characters cleared of IAC commands, 1 line at a time
                    char __prompt__ = 0;
                    uint16_t __clientWindowWidth__ = 0;
                    uint16_t __clientWindowHeight__ = 0;
                    bool __echo__ = true;

                    static UBaseType_t __lastHighWaterMark__;


                public:

                    telnetConnection_t (
                                        #ifdef __THREAD_SAFE_FS__
                                            threadSafeFS::FS fileSystem,                                                                    // file system that FTP server would use
                                        #endif
                                        Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password),
                                        int connectionSocket,                                                                       // socket number
                                        char *clientIP,                                                                             // client's IP
                                        char *serverIP,                                                                             // server's IP (the address to which the connection arrived)
                                        String (*telnetCommandHandlerCallback) (int argc, char *argv  [], telnetConnection_t *tcn)  // telnetCommandHandlerCallback function provided by calling program
                                       ) : tcpConnection_t (connectionSocket, clientIP, serverIP),
                                           #ifdef __THREAD_SAFE_FS__
                                               __fileSystem__ (fileSystem),
                                           #endif
                                           __getUserHomeDirectory__ (getUserHomeDirectory),
                                           __telnetCommandHandlerCallback__ (telnetCommandHandlerCallback) {
                    }

                    // telnet session related variables

                    inline char *getUserName () __attribute__((always_inline)) { return __userName__; }
                    #ifdef __THREAD_SAFE_FS__
                        inline char *getHomeDir () __attribute__((always_inline)) { return __homeDirectory__; }
                        inline char *getWorkingDir () __attribute__((always_inline)) { return __workingDirectory__; }
                    #endif
                    inline uint16_t getClientWindowWidth () __attribute__((always_inline)) { return __clientWindowWidth__; }
                    inline uint16_t getClientWindowHeight () __attribute__((always_inline)) { return __clientWindowHeight__; }

                    // telnet connection functions

                    char recvChar (bool peekOnly = false) { // returns valid character or 0 in case of error, extracts IAC commands from stream
                        unsigned char c;
                        while (true) {

                            if (peekOnly) { // only peek
                                if (__peekedChar__) {
                                    return __peekedChar__;
                                } else {
                                    switch (peek (&c, 1)) {
                                        case -1:  // error
                                                  return 3; // Ctrl-C, wahtever different than 0 so that the calling program will readChar afterwards and detect the error 
                                        case 0:   // no data available to be read
                                                  return 0;
                                        default:  // c is waiting to be read
                                                  break; // continue reading and processing c
                                    }
                                }
                            } else { // true read
                                if (__peekedChar__) {
                                      c = __peekedChar__;
                                      __peekedChar__ = 0;
                                      return (char) c;
                                }

                            }

                            // read the next charcter
                            if (recvBlock (&c, 1) <= 0) return 0;

                            // process character and (some) IAC commands
                            switch (c) {
                                case 3:       break;            // Ctrl-C
                                case 4:                         // Ctrl-D
                                case 26:      c = 4; break;     // Ctrl-Z
                                #ifdef SWITHC_DEL_AND_BACKSPACE
                                    // windows Telnet terminal: BkSpace key = 8, Del key = 127, Putty, Linux: Del key = 127, BkSpace key = ESC sequence: 27, 91, 51, 126
                                    case 8:   c = 127; break;   // BkSpace
                                    case 127: c = 8; break;     // Del
                                #else
                                    // windows Telnet terminal: BkSpace key = 8, Del key = 127, Putty, Linux: Del key = 127, BkSpace key = ESC sequence: 27, 91, 51, 126
                                    case 8:                     // BkSpace
                                    case 127:                   // Del
                                #endif
                                case 9:                         // Tab
                                case 13:      break;            // Enter
                                case __IAC__:       // read the next character
                                                    if (recvBlock (&c, 1) <= 0) return 0;

                                                    switch (c) {
                                                        case __SB__:  // read the next character
                                                                      if (recvBlock (&c, 1) <= 0) return 0;
                                                                      if (c == __NAWS__) { 
                                                                          // read the next 4 bytes indicating client window size
                                                                          char chars [4];
                                                                          if (recvBlock (chars, sizeof (chars)) != sizeof (chars)) return 0;                          
                                                                          __clientWindowWidth__ = (uint16_t) chars [0] << 8 | (uint8_t) chars [1]; 
                                                                          __clientWindowHeight__ = (uint16_t) chars [2] << 8 | (uint8_t) chars [3];
                                                                      } 
                                                                      // read the rest of IAC command until SE
                                                                      while (c != __SE__)
                                                                          if (recvBlock (&c, 1) <= 0) return 0;
                                                                      continue;
                                                      // in the following cases the 3rd character is following, ignore this one too
                                                      case __WILL__:  
                                                      case __WONT__:  
                                                      case __DONT__:  if (recvBlock (&c, 1) <= 0) return 0;
                                                                      continue;
                                                      case __DO__:    if (recvBlock (&c, 1) <= 0) return 0;
                                                                      if (c == __CHARSET__) {
                                                                          int i = sendString ((char *) IAC SB CHARSET REQUEST "UTF-8" IAC SE);
                                                                          if (i <= 0) {
                                                                              dmesgQueue << "[telnetConn] send error: " << errno << " " << strerror (errno);
                                                                          }
                                                                      }
                                                      default:        // ignore
                                                                      continue;
                                                  } // switch level 2, after IAC
                                                  c = 0; // invalidate the character beeing read since it was only the part of IAC command
                                                  break;              
                                default:    //  ignore LF that Telnet client send after CR when Enter is pressed
                                            if (c == 10)
                                                c = 0;
                                            break;
                            } // switch level 1
                            __peekedChar__ = c;
                        } // while
                        return 0; // never executes
                    }

                    char peekChar () { return recvChar (true); } // returns the next character to be read or 0 if none is pending to be read (readChar should follow to detect possible errors)

                    char recvLine (char *buf, size_t len, bool trim = true) { // reads incoming stream, returns last character read or 0-error, 3-Ctrl-C, 4-EOF, 10-Enter, ...
                        if (!len) 
                            return 0;
                        int characters = 0;
                        buf [0] = 0;

                        while (true) { // read and process incomming data in a loop 
                          char c = recvChar ();
                          switch (c) {
                              case 0:   return 0; // Error
                              case 3:   return 3; // Ctrl-C
                              case 4:             // EOF - Ctrl-D (UNIX)
                              case 26:  return 4; // EOF - Ctrl-Z (Windows)
                              // windows Telnet terminal: BkSpace key = 8, Del key = 127, Putty, Linux: Del key = 127, BkSpace key = ESC sequence: ESC[3~
                              case 8:   // backspace - delete last character from the buffer and from the screen
                              case 127: // in Windows telent.exe this is del key, but putty reports backspace with code 127, so let's treat it the same way as backspace
                                        if (characters && buf [characters - 1] >= ' ') {
                                            buf [-- characters] = 0;
                                            if (__echo__ && sendString ("\x08 \x08") <= 0) 
                                                return 0; // delete the last character from the screen
                                        }
                                        break;   
                              case 27: 
                                        if (!(c = recvChar ())) 
                                            return 0;
                                        if (c == '[') { // ESC [
                                            if (!(c = recvChar ())) 
                                                return 0;
                                            if (c == '3') { // ESC [3
                                                if (!(c = recvChar ())) 
                                                    return 0;
                                                if (c == '~') {
                                                    if (characters && buf [characters - 1] >= ' ') {
                                                        buf [-- characters] = 0;
                                                        if (__echo__ && sendString ("\x08 \x08") <= 0) 
                                                            return 0; // delete the last character from the screen
                                                    }
                                                }
                                            }
                                        }
                                        break; // ignore all the rest sequences
                              case 10:  // LF
                                        break; // ignore
                              case 13:  // CR
                                        if (trim) {
                                            while (buf [0] == ' ') strcpy (buf, buf + 1); // ltrim
                                            int i; for (i = 0; buf [i] > ' '; i++); buf [i] = 0; // rtrim
                                        }
                                        if (__echo__ && sendString ("\r\n") <= 0) 
                                            return 0; // echo CRLF to the screen
                                        return 13;
                              case 9:   // tab - treat as 2 spaces
                                        c = ' ';
                                        if (characters < len - 1) {
                                            buf [characters] = c; buf [++ characters] = 0;
                                            if (__echo__ && sendBlock (&c, sizeof (c)) <= 0) 
                                                return 0; // echo last character to the screen
                                        }                
                                        // continue with default (repeat adding a space):
                              default:  // fill the buffer 
                                        if (characters < len - 1) {
                                            buf [characters] = c; buf [++ characters] = 0;
                                            if (__echo__ && sendBlock (&c, sizeof (c)) <= 0) 
                                                return 0; // echo last character to the screen
                                        }
                                        break;              
                          } // switch
                        } // while
                        return 0;
                    }


                private:

                    void __runConnectionTask__ () {

                        // ----- this is where Telnet connection starts, the socket is already opened -----

                            ///#if USER_MANAGEMENT == NO_USER_MANAGEMENT
                            if (__getUserHomeDirectory__ == NULL) {

                                // tell the client to go into character mode, not to echo and send back its window size, then say hello 
                                sprintf (__cmdLine__, IAC WILL ECHO IAC WILL SUPPRESS_GO_AHEAD IAC DO NAWS HOSTNAME " says hello to %s.\r\n", getClientIP ());
                                if (sendString (__cmdLine__) <= 0)
                                    return;

                                // prepare Telnet session variables
                                __userName__ = "root";
                                __homeDirectory__ = "/";
                                __workingDirectory__ =  "/";
                                __prompt__ = '#';

                            } else {

                                sprintf (__cmdLine__, IAC WILL ECHO IAC WILL SUPPRESS_GO_AHEAD IAC DO NAWS HOSTNAME " says hello to %s, please login.\r\nuser: ", getClientIP ());
                                if (sendString (__cmdLine__) <= 0) 
                                    return;
                                if (recvLine (__userName__, 64) != 13)
                                    return; 
                                Cstring<64> password;
                                __echo__ = false;
                                if (sendString ("password: ") <= 0)
                                    return;
                                if (recvLine (password, 64) != 13)
                                    return;
                                __echo__ = true;

                                // check user name and password and get user's home directory, prepare Telnet session variables
                                __homeDirectory__ = __getUserHomeDirectory__ (__userName__, password);
                                if (__homeDirectory__ == "") {
                                    dmesgQueue << "[telnetConn] login denyed for " << __userName__;
                                    sendString ("\r\nUsername and/or password incorrect");
                                    delay (100);
                                    return;
                                }
                                __workingDirectory__ = __homeDirectory__;
                                __prompt__ = __userName__ == "root" ? '#' : '$';

                            }
                            dmesgQueue << "[telnetConn] " << __userName__ << " logged in";

                            sprintf (__cmdLine__, "\r\nWelcome %s, use \"help\" to display available commands.\r\n\n", __userName__);
                            if (sendString (__cmdLine__) <= 0)
                                return;
                            __cmdLine__ [0] = 0;

                            // notify callback handler procedure with special SESSION START command 
                            char *sessionState = (char *) "SESSION START";
                            if (__telnetCommandHandlerCallback__) 
                                __telnetCommandHandlerCallback__ (1, &sessionState, this);

                            while (true) { // endless loop of reading and executing commands
                                // write prompt and possibly \r\n
                                sprintf (__cmdLine__ + strlen (__cmdLine__), "%c ", __prompt__);
                                if (sendString (__cmdLine__) <= 0) 
                                    goto endConnection;

                                switch (recvLine (__cmdLine__, sizeof (__cmdLine__), false)) {
                                    case 3:   {   // Ctrl-C, end the connection
                                                  sendString ("\r\nCtrl-C");
                                                  goto endConnection;
                                              }
                                    case 13:  {   // Enter, parse command line
                                                    int argc = 0;
                                                    char *argv [TELNET_SESSION_MAX_ARGC];
                                                    char *p = __cmdLine__;
                                                    bool finished = false;
                                                    while (!finished && argc <= TELNET_SESSION_MAX_ARGC) {
                                                        while (*p == ' ')
                                                            p++;
                                                        if (*p) {
                                                            argv [argc++] = p;
                                                            bool insideQuotes = false;
                                                            while (*p) {
                                                                if (*p == '\"') {
                                                                    insideQuotes = !insideQuotes;
                                                                    p++;
                                                                } else if (*p == ' ' && !insideQuotes) {
                                                                    *p = 0;
                                                                    p++;
                                                                    break;
                                                                } else if (*p == 0) {
                                                                    finished = true;
                                                                    break;
                                                                } else {
                                                                    p++;
                                                                }
                                                            }
                                                        } else {
                                                            finished = true;
                                                        }
                                                    }

                                                  // process commandLine
                                                  if (argc) {
                                                      // ask telnetCommandHandler (if it is provided by the calling program) if it is going to handle this command, otherwise try to handle it internally
                                                      String s;
                                                      if (__telnetCommandHandlerCallback__) 
                                                          s = __telnetCommandHandlerCallback__ (argc, argv, this);

                                                      if (!s) { // out of memory                         
                                                          if (sendString ("Out of memory") <= 0) 
                                                              goto endConnection;
                                                      } else if (s != "") { // __telnetCommandHandlerCallback__ returned a reply                
                                                          if (sendString (s.c_str ()) <= 0) 
                                                              goto endConnection;
                                                      } else {

                                                          // __telnetCommandHandlerCallback__ returned "" - handle the command internally
                                                          Cstring<300> s = internalCommandHandler (argc, argv);

                                                          if (getSocket () == -1) 
                                                              goto endConnection; // in case of quit - quit command closes the socket itself
                                                          if (s != "") { 
                                                              if (sendString (s) <= 0) 
                                                                  goto endConnection;
                                                          } else {
                                                              if (sendString ("Invalid command, use \"help\" to display available commands") <= 0) 
                                                                  goto endConnection;
                                                          }
                                                      }
                                                      sprintf (__cmdLine__, "\r\n"); // will be sent to client together with promtp sign  
                                                  } else {
                                                      __cmdLine__ [0] = 0; // prepare buffer for the next user input                           
                                                  }
                                                  break;
                                              }
                                    case 0:   {
                                                  if (errno == EAGAIN || errno == ENAVAIL) 
                                                      sendString ("\r\ntimeout");
                                                  goto endConnection;                
                                              }
                                    default:  // ignore
                                              break;
                                }


                                // check how much stack did we use
                                UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
                                if (__lastHighWaterMark__ > highWaterMark) {
                                    dmesgQueue << "[telnetConn] new Telnet connection stack high water mark reached: " << highWaterMark << " not used bytes";
                                    __lastHighWaterMark__ = highWaterMark;
                                }


                            } // endless loop of reading and executing commands
                    endConnection:

                            // notify callback handler procedure with special SESSION END command
                            sessionState = (char *) "SESSION END";
                            if (__telnetCommandHandlerCallback__) 
                                __telnetCommandHandlerCallback__ (1, &sessionState, this);

                            if (__prompt__) 
                                dmesgQueue << "[telnetConn] " << __userName__ << " logged out"; // if prompt is set, we know that login was successful

                        // ----- this is where Telnet connection ends, the socket will be closed upon return -----

                    }

                    Cstring<300> internalCommandHandler (int argc, char *argv []) {
                        #define telnetArgv0Is(X) (argc > 0 && !strcmp (argv [0], X))
                        #define telnetArgv1Is(X) (argc > 1 && !strcmp (argv [1], X))

                        // for (int i = 0; i < argc; i++) log_i ("argv [%i] = %s\n", i, argv [i]);                  

                        // help command is the only command that gets always included
                        if (telnetArgv0Is ("help"))                 { return argc == 1 ? __help__ () : "Wrong syntax, use help"; }

                        #if TELNET_CLEAR_COMMAND == 1
                            else if (telnetArgv0Is ("clear"))       { return argc == 1 ? __clear__ () : "Wrong syntax, use clear"; }
                        #endif

                        #if TELNET_UNAME_COMMAND == 1 
                            else if (telnetArgv0Is ("uname"))       { return argc == 1 ? __uname__ () : "Wrong syntax, use uname"; }
                        #endif

                        #if TELNET_FREE_COMMAND == 1
                            else if (telnetArgv0Is ("free"))        { 
                                                                        if (argc == 1)                                          return __free__ (0);
                                                                        if (argc == 2) {
                                                                            int n = atoi (argv [1]); if (n > 0 && n <= 3600)    return __free__ (n);
                                                                        }
                                                                                                                                return "Wrong syntax, use free [<n>]   (where 0 < n <= 3600)";
                                                                    } 
                        #endif

                        #if TELNET_NOHUP_COMMAND == 1
                            else if (telnetArgv0Is ("nohup"))       {
                                                                        if (argc == 1)                                          return __nohup__ (0);
                                                                        if (argc == 2) {
                                                                            int n = atoi (argv [1]); if (n > 0 && n < 3600)     return __nohup__ (n);
                                                                        }
                                                                                                                                return "Wrong syntax, use nohup [<n>]   (where 0 < n <= 3600)";
                                                                    }
                        #endif

                        #if TELNET_REBOOT_COMMAND == 1
                            else if (telnetArgv0Is ("reboot"))      {
                                                                        if (argc == 1)                                          return __reboot__ (true);
                                                                        if (telnetArgv1Is ("-h") || telnetArgv1Is ("-hard"))    return __reboot__ (false);
                                                                                                                                return "Wrong syntax, use reboot [-hard]"; 
                                                                    }
                        #endif

                        #if TELNET_DMESG_COMMAND == 1
                            else if (telnetArgv0Is ("dmesg"))     { 
                                                                      bool f = false;
                                                                      bool t = false;
                                                                      for (int i = 1; i < argc; i++) {
                                                                              if (!strcmp (argv [i], "-f") || !strcmp (argv [i], "-follow")) f = true;
                                                                          else if (!strcmp (argv [i], "-t") || !strcmp (argv [i], "-time")) t = true;
                                                                          else return "Wrong syntax, use dmesg [-follow] [-time]";
                                                                      }
                                                                      return __dmesg__ (f, t);
                                                                  }
                                                        
                        #endif

                        #if TELNET_QUIT_COMMAND == 1                                                  
                            else if (telnetArgv0Is ("quit"))        { return argc == 1 ? __quit__ () : "Wrong syntax, use quit"; }
                        #endif

                        #if TELNET_UPTIME_COMMAND == 1          
                            else if (telnetArgv0Is ("uptime"))      { return argc == 1 ? __uptime__ () : "Wrong syntax, use uptime"; }
                        #endif

                        /*
                        #ifdef 
                            else if (telnetArgv0Is ("ntpdate"))   {
                                                                      if (argc == 1)  return __ntpdate__ ();
                                                                      if (argc == 2)  return __ntpdate__ (argv [1]);
                                                                                      return "Wrong syntax, use ntpdate [ntpServer]";
                                                                  }

                            else if (telnetArgv0Is ("crontab"))   { return argc == 1 ? __cronTab__ () : "Wrong syntax, use crontab"; }
                        #endif
                        */

                        #if TELNET_PING_COMMAND == 1
                            else if (telnetArgv0Is ("ping"))        { 
                                                                        if (argc == 2) return __ping__ (argv [1]);
                                                                        return "Wrong syntax, use ping <target computer>"; 
                                                                    }
                        #endif

                        #if TELNET_IFCONFIG_COMMAND == 1
                            else if (telnetArgv0Is ("ifconfig"))    {
                                                                        if (argc == 1) return __ifconfig__ ();
                                                                        return "Wrong syntax, use ifconfig";              
                                                                    }
                        #endif

                        #if TELNET_IW_COMMAND == 1
                            else if (telnetArgv0Is ("iw"))           {
                                                                        if (argc == 1) return __iw__ ();
                                                                        return "Wrong syntax, use iw";              
                                                                    }
                        #endif

                        #if TELNET_NETSTAT_COMMAND == 1
                            else if (telnetArgv0Is ("netstat"))     {
                                                                        if (argc == 1)                                          return __netstat__ (0);
                                                                        if (argc == 2) {
                                                                            int n = atoi (argv [1]); if (n > 0 && n < 3600)     return __netstat__ (n);
                                                                        }
                                                                                                                                return "Wrong syntax, use netstat [<n>]   (where 0 < n <= 3600)";
                                                                    }
                        #endif

                        #if TELNET_KILL_COMMAND == 1
                            else if (telnetArgv0Is ("kill"))          { 
                                                                        if (!strcmp (__userName__, "root")) {
                                                                            if (argc == 2) {
                                                                                int n = atoi (argv [1]); if (n >= LWIP_SOCKET_OFFSET && n < LWIP_SOCKET_OFFSET + MEMP_NUM_NETCONN) return __kill__ (n);
                                                                            }
                                                                            return Cstring<300> ("Wrong syntax, use kill <socket>   (where ") + Cstring<300> (LWIP_SOCKET_OFFSET) + " <= socket <= " + Cstring<64> (LWIP_SOCKET_OFFSET + MEMP_NUM_NETCONN - 1) + ")";
                                                                        }
                                                                        else return "Only root may close sockets";
                                                                    }
                        #endif
                                                    
                        #if TELNET_CURL_COMMAND == 1
                            else if (telnetArgv0Is ("curl"))        { 
                                                                        if (argc == 2)  return __curl__ ("GET", argv [1]);
                                                                        if (argc == 3)  return __curl__ (argv [1], argv [2]);
                                                                                        return "Wrong syntax, use curl [method] http://url";
                                                                    } 
                        #endif

                        #if TELNET_SENDMAIL_COMMAND == 1
                            else if (telnetArgv0Is ("sendmail"))    { 
                                // for example: sendmail -m "This is the message." -s "This si the subject" -t \"Someone\"<some.one@somewhere.com> -f \"Somebody\"somebody@somewhere.net -p password -u user -P 25 -S smtp.somewhere.net
                                                                        const char *smtpServer = "";
                                                                        const char *smtpPort = "";
                                                                        const char *userName = "";
                                                                        const char *password = "";
                                                                        const char *from = "";
                                                                        const char *to = ""; 
                                                                        const char *subject = ""; 
                                                                        const char *message = "";
                                                                    
                                                                        for (int i = 1; i < argc - 1; i += 2) {           
                                                                                if (!strcmp (argv [i], "-S")) smtpServer = argv [i + 1];
                                                                            else if (!strcmp (argv [i], "-P")) smtpPort = argv [i + 1];
                                                                            else if (!strcmp (argv [i], "-u")) userName = argv [i + 1];
                                                                            else if (!strcmp (argv [i], "-p")) password = argv [i + 1];
                                                                            else if (!strcmp (argv [i], "-f")) from = argv [i + 1];
                                                                            else if (!strcmp (argv [i], "-t")) to = argv [i + 1];
                                                                            else if (!strcmp (argv [i], "-s")) subject = argv [i + 1];
                                                                            else if (!strcmp (argv [i], "-m")) message = argv [i + 1];
                                                                            #ifdef __THREAD_SAFE_FS__
                                                                                else return "Wrong syntax, use sendmail [-S smtpSrv] [-P smtpPort] [-u usrNme] [-p passwd] [-f from addr] [-t to addr list] [-s subject] [-m msg]";
                                                                            #else
                                                                                else return "Wrong syntax, use sendmail -S smtpSrv -P smtpPort -u usrNme -p passwd -f from addr -t to addr list -s subject -m msg";
                                                                            #endif
                                                                        }

                                                                        #ifdef __THREAD_SAFE_FS__
                                                                            return sendMail (message, subject, to, from, password, userName, atoi (smtpPort), smtpServer);
                                                                        #else
                                                                            return __sendMail__ (message, subject, to, from, password, userName, atoi (smtpPort), smtpServer);
                                                                        #endif
                                                                    }
                        #endif

                        /*
                        else if (telnetArgv0Is ("date"))          {
                                                                      if (argc == 1)  return __getDateTime__ ();
                                                                      if (argc > 2 && (telnetArgv1Is ("-s") || telnetArgv1Is ("-set"))) {
                                                                          // reconstruct date from command line that has already been parsed
                                                                          for (char *p = argv [2]; p < argv [argc - 1]; p++)
                                                                              if (!*p)
                                                                                  *p = ' ';
                                                                          return __setDateTime__ (argv [2]);
                                                                      }
                                                                      return "Wrong syntax, use date [-set <date-time>]";
                                                                  }

                        */



                        #if TELNET_LS_COMMAND == 1
                            else if (telnetArgv0Is ("ls"))          {
                                                                        if (argc == 1)  return __ls__ (__workingDirectory__);
                                                                        if (argc == 2)  return __ls__ (argv [1]);
                                                                                        return "Wrong syntax, use ls [<directoryName>]";
                                                                    }
                        #endif

                        #if TELNET_TREE_COMMAND == 1
                            else if (telnetArgv0Is ("tree"))        {
                                                                        if (argc == 1)  return __tree__ (__workingDirectory__); 
                                                                        if (argc == 2)  return __tree__ (argv [1]); 
                                                                                        return "Wrong syntax, use tree [<directoryName>]";
                                                                    }
                        #endif

                        #if TELNET_MKDIR_COMMAND == 1
                            else if (telnetArgv0Is ("mkdir"))       { return argc == 2 ? __mkdir__ (argv [1]) : "Wrong syntax, use mkdir <directoryName>"; } 
                        #endif

                        #if TELNET_RMDIR_COMMAND == 1
                            else if (telnetArgv0Is ("rmdir"))       { return argc == 2 ? __rmdir__ (argv [1]) : "Wrong syntax, use rmdir <directoryName>"; } 
                        #endif

                        #if TELNET_CD_COMMAND == 1
                            else if (telnetArgv0Is ("cd"))          { return argc == 2 ? __cd__ (argv [1]) : "Wrong syntax, use cd <directoryName>"; }
                            else if (telnetArgv0Is ("cd.."))        { return argc == 1 ? __cd__ ("..") : "Wrong syntax, use cd <directoryName>"; } 
                        #endif

                        #if TELNET_PWD_COMMAND == 1
                            else if (telnetArgv0Is ("pwd"))         { return argc == 1 ? __pwd__ () : "Wrong syntax, use pwd"; }
                        #endif

                        #if TELNET_CAT_COMMAND == 1
                            else if (telnetArgv0Is ("cat"))         {
                                                                        if (argc == 2)                          return __catFileToClient__ (argv [1]);
                                                                        if (argc == 3 && telnetArgv1Is (">"))   return __catClientToFile__ (argv [2]);
                                                                                                                return "Wrong syntax, use cat [>] <fileName>";
                                                                    }
                        #endif

                        #if TELNET_VI_COMMAND == 1
                            else if (telnetArgv0Is ("vi"))          {  
                                                                        if (argc == 2) {
                                                                            time_t seconds;
                                                                            seconds = getIdleTimeout ();
                                                                            setIdleTimeout (0); // infinite
                                                                            cstring s = __vi__ (argv [1]);
                                                                            setIdleTimeout (seconds);
                                                                            return s;
                                                                        }
                                                                        return "Wrong syntax, use vi <fileName>";
                                                                    }
                        #endif

                        #if TELNET_CP_COMMAND == 1
                            else if (telnetArgv0Is ("cp"))          { return argc == 3 ? __cp__ (argv [1], argv [2]) : "Wrong syntax, use cp <existing fileName> <new fileName>"; }
                        #endif

                        #if TELNET_RM_COMMAND == 1
                            else if (telnetArgv0Is ("rm"))          { return  argc == 2 ? __rm__ (argv [1]) : "Wrong syntax, use rm <fileName>"; }
                        #endif

                        /*
                
                            else if (telnetArgv0Is ("useradd")) { 
                                                              if (strcmp (__userName__, "root"))                                      return "Only root may add users";
                                                              if (argc == 6 && !strcmp (argv [1], "-u") && !strcmp (argv [3], "-d"))  return userManagement.userAdd (argv [5], argv [2], argv [4]);
                                                                                                                                      return "Wrong syntax, use useradd -u userId -d userHomeDirectory userName (where userId > 1000)";
                                                          }
                                                          
                            else if (telnetArgv0Is ("userdel")) { 
                                                              if (strcmp (__userName__, "root"))  return "Only root may delete users";
                                                              if (argc != 2)                      return "Wrong syntax. Use userdel userName";
                                                              if (!strcmp (argv [1], "root"))     return "You don't really want to to this";
                                                                                                  return userManagement.userDel (argv [1]);
                                                          }
                  
                            else if (telnetArgv0Is ("passwd"))  { 
                                                              if (argc == 1)                                                              return __passwd__ (__userName__);
                                                              if (argc == 2) {
                                                                if (!strcmp (__userName__, "root") || !strcmp (argv [1], __userName__))   return __passwd__ (argv [1]); 
                                                                else                                                                      return "You may change only your own password";
                                                              }
                                                                                                                                          return "";
                                                          }
                                                                  
                        */

                        return "";
                    }

                    // help command is the only command that always included
                    const char *__help__ () {
                        #ifndef __USER_DEFINED_TELNET_HELP_TEXT__
                            #define __USER_DEFINED_TELNET_HELP_TEXT__ ""
                        #endif
                        const char *telnetHelpText =    "Suported commands:"
                                                        "\r\n      help"
                                                        #if TELNET_CLEAR_COMMAND == 1
                                                            "\r\n      clear"
                                                        #endif
                                                        #if TELNET_UNAME_COMMAND == 1
                                                            "\r\n      uname"
                                                        #endif
                                                        #if TELNET_FREE_COMMAND == 1
                                                            "\r\n      free [<n>]    (where 0 < n <= 3600)"
                                                        #endif
                                                        #if TELNET_NOHUP_COMMAND == 1
                                                            "\r\n      nohup [<n>]   (where 0 < n <= 3600)"
                                                        #endif
                                                        #if TELNET_REBOOT_COMMAND == 1
                                                            "\r\n      reboot [-h]   (-h for hard reset)"
                                                        #endif
                                                        #if TELNET_DMESG_COMMAND == 1
                                                            "\r\n      dmesg [-follow] [-time]"
                                                        #endif
                                                        #if TELNET_QUIT_COMMAND == 1
                                                            "\r\n      quit"
                                                        #endif
                                                        #if TELNET_UPTIME_COMMAND == 1 or TELNET_DATE_COMMAND == 1
                                                            "\r\n  time commands:"
                                                        #endif
                                                        #if TELNET_UPTIME_COMMAND == 1                           
                                                            "\r\n      uptime"
                                                        #endif
                                                        #if TELNET_DATE_COMMAND == 1
                                                            "\r\n      date [-set <date-time>]"
                                                        #endif
                                                        
                                                        /*
                                                        #ifdef __TIME_FUNCTIONS__                                
                                                            "\r\n      ntpdate"
                                                            "\r\n      crontab"
                                                        #endif
                                                        */
                                                        #if TELNET_PING_COMMAND == 1 or TELNET_IFCONFIG_COMMAND == 1 or TELNET_NETSTATS_COMMAND == 1 or TELNET_KILL_COMMAND == 1 or TELNET_CURL_COMMAND == 1 or TELNET_SENDMAIL_COMMAND == 1
                                                            "\r\n  network commands:"
                                                        #endif
                                                        #if TELNET_PING_COMMAND == 1
                                                            "\r\n      ping <terget computer>"
                                                        #endif
                                                        #if TELNET_IFCONFIG_COMMAND == 1
                                                            "\r\n      ifconfig"
                                                        #endif
                                                        #if TELNET_IW_COMMAND == 1
                                                            "\r\n      iw"
                                                        #endif
                                                        #if TELNET_NETSTAT_COMMAND == 1
                                                            "\r\n      netstat [<n>]   (where 0 < n <= 3600)"
                                                        #endif
                                                        #if TELNET_KILL_COMMAND == 1
                                                            "\r\n      kill <socket>   (where socket is a valid socket)"
                                                        #endif
                                                        #if TELNET_CURL_COMMAND == 1
                                                            "\r\n      curl [method] http://url"
                                                        #endif
                                                        #if TELNET_SENDMAIL_COMMAND == 1
                                                            #ifdef __THREAD_SAFE_FS__
                                                                "\r\n      sendmail [-S smtpSrv] [-P smtpPort] [-u usrNme] [-p passwd] [-f from addr] [-t to addr list] [-s subject] [-m msg]"
                                                                "\r\n      (the default argument values can be stored in /etc/mail/sendmail.cf file)"
                                                            #else
                                                                "\r\n      sendmail -S smtpSrv -P smtpPort -u usrNme -p passwd -f from addr -t to addr list -s subject -m msg"
                                                            #endif
                                                        #endif

                                                        #if TELNET_LS_COMMAND == 1 or TELNET_TREE_COMMAND == 1 or TELNET_MKDIR_COMMAND == 1 or TELNET_RMDIR_COMMAND == 1 or TELNET_CD_COMMAND == 1 or TELNET_PWD_COMMAND == 1 or TELNET_CAT_COMMAND == 1 or TELNET_VI_COMMAND == 1 or  TELNET_CP_COMMAND == 1 or  TELNET_RM_COMMAND == 1
                                                            "\r\n  file commands:"
                                                        #endif
                                                        #if TELNET_LS_COMMAND == 1
                                                            "\r\n      ls [<directoryName>]"
                                                        #endif
                                                        #if TELNET_TREE_COMMAND == 1
                                                            "\r\n      tree [<directoryName>]"
                                                        #endif
                                                        #if TELNET_MKDIR_COMMAND == 1
                                                            "\r\n      mkdir <directoryName>"
                                                        #endif
                                                        #if TELNET_RMDIR_COMMAND == 1
                                                            "\r\n      rmdir <directoryName>"
                                                        #endif
                                                        #if TELNET_CD_COMMAND == 1
                                                            "\r\n      cd <directoryName or ..>"
                                                        #endif
                                                        #if TELNET_PWD_COMMAND == 1
                                                            "\r\n      pwd"
                                                        #endif
                                                        #if TELNET_CAT_COMMAND == 1
                                                            "\r\n      cat [>] <fileName>"
                                                        #endif
                                                        #if TELNET_VI_COMMAND == 1
                                                            "\r\n      vi <fileName>"
                                                        #endif
                                                        #if TELNET_CP_COMMAND == 1
                                                            "\r\n      cp <existing fileName> <new fileName>"
                                                        #endif
                                                        #if TELNET_RM_COMMAND == 1
                                                            "\r\n      rm <fileName>"
                                                        #endif
                                                        __USER_DEFINED_TELNET_HELP_TEXT__;

                        sendString (telnetHelpText);
                        return "\r"; // different than "" to let the calling function know that the command has been processed
                    }

                    #if TELNET_CLEAR_COMMAND == 1
                        const char *__clear__ () { return "\x1b[2J"; } // ESC[2J = clear screen
                    #endif

                    #if TELNET_UNAME_COMMAND == 1
                        #include "version_of_servers.h"
                        Cstring<300> __uname__ () { return Cstring<300> (MACHINETYPE " (") + Cstring<300> ((int) ESP.getCpuFreqMHz ()) + " MHz) " HOSTNAME " SDK: " + ESP.getSdkVersion () + " " VERSION_OF_SERVERS " compiled: " __DATE__ " " __TIME__  + " C++: " + Cstring<64> ((unsigned int) __cplusplus); }
                    #endif

                    #if TELNET_FREE_COMMAND == 1
                        const char *__free__ (unsigned long delaySeconds) {
                            bool firstTime = true;
                            int currentLine = 0;
                            char s [128];
                            do { // follow         
                                if (firstTime || (getClientWindowHeight ()  && currentLine >= getClientWindowHeight ())) {
                                    sprintf (s, "%sFree heap       Max block Free PSRAM\r\n-------------------------------------------", firstTime ? "" : "\r\n");
                                    if (sendString (s) <= 0) 
                                        return "";
                                    currentLine = 2; // we have just displayed 2 lines (header)
                                }
                                if (!firstTime && delaySeconds) { // wait and check if user pressed a key
                                    unsigned long startMillis = millis ();
                                    while (millis () - startMillis < (delaySeconds * 1000)) {
                                        delay (100);
                                        if (peekChar ()) {
                                            recvChar ();
                                            return "\r"; // return if user pressed Ctrl-C or any key
                                        } 
                                    }
                                }
                                sprintf (s, "\r\n%10lu   %10lu   %10lu  bytes", (unsigned long) ESP.getFreeHeap (), (unsigned long) heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT), (unsigned long) ESP.getFreePsram ());
                                if (sendString (s) <= 0) 
                                    return "";
                                firstTime = false;
                                currentLine ++; // we have just displayed the next line (data)
                            } while (delaySeconds);
                            return "\r"; // different than "" to let the calling function know that the command has been processed
                        }
                    #endif

                    #if TELNET_NOHUP_COMMAND == 1
                        Cstring<300> __nohup__ (long timeOutSeconds) {
                            setIdleTimeout (timeOutSeconds);
                            return timeOutSeconds ? Cstring<300> ("The connection timeout is ") + Cstring<300> (timeOutSeconds) + " seconds" : "The connection timeout is infinite";
                        }
                    #endif

                    #if TELNET_REBOOT_COMMAND == 1
                        const char *__reboot__ (bool softReboot) {
                            if (softReboot) {
                                sendString ("(Soft) rebooting ...");
                                delay (250);
                                ESP.restart ();
                            } else {
                                // cause WDT reset
                                sendString ("(Hard) rebooting ...");
                                delay (250);
                                esp_task_wdt_config_t wdtCfg = {0, 1 << 1, true};
                                esp_task_wdt_init (&wdtCfg);
                                esp_task_wdt_add (NULL);
                                while (true);
                            }
                            return "";
                        }
                    #endif

                    #if TELNET_DMESG_COMMAND == 1
                        #ifdef __DMESG__
                            const char * __dmesg__ (bool follow, bool trueTime) {
                                String s;
                                bool outOfMemory = false;

                                bool firstRecord = true;
                                dmesgQueueEntry_t *lastBack = NULL;
                                for (auto e: dmesgQueue) { // scan dmesgQueue with iterator where the locking mechanism is already implemented

                                    if (firstRecord) firstRecord = false; else if (!s.concat ("\r\n")) outOfMemory = true;
                                    char c [80];
                                    if (trueTime && e.time > 1687461154) { // 2023/06/22 21:12:34, any valid time should be greater than this
                                        struct tm slt; 
                                        localtime_r (&e.time, &slt); 
                                        #ifndef __LOCALE_HPP__
                                            strftime (c, sizeof (c), "[%Y/%m/%d %T] ", &slt);
                                        #else
                                            Cstring<64> f = "["; f += lc_time_locale->getTimeFormat (); f += "] ";
                                            strftime (c, sizeof (c), f, &slt);
                                        #endif
                                    } else {
                                        sprintf (c, "[%10lu] ", e.milliseconds);
                                    }
                                    if (!s.concat (c)) outOfMemory = true;
                                    if (!s.concat (e.message)) outOfMemory = true;
                                    if (e.message.errorFlags () & err_overflow)
                                        if (!s.concat ("...")) outOfMemory = true;
                                    lastBack = &dmesgQueue.back (); // remember the last change in case -f is specified
                                }
                                if (sendString (s.c_str ()) <= 0)
                                    return "\r";
                                if (outOfMemory)
                                    return "Out of memory";

                                // -follow?
                                while (follow) {
                                    delay (100);

                                    // stop waiting if a key is pressed
                                    if (peekChar ()) {
                                        recvChar (); 
                                        return "\r"; // return if user pressed Ctrl-C or any key
                                    } 

                                    // read the new dmsg entries if they appeared
                                    if (lastBack != &dmesgQueue.back ()) { // changed
                                        // find where we have finished previous time
                                        auto e = dmesgQueue.begin ();
                                        do {
                                            ++ e;
                                        } while (e != dmesgQueue.end () && &*e != lastBack);
                                        if (e != dmesgQueue.end ())
                                            ++ e;

                                        while (e != dmesgQueue.end ()) {
                                            s = "\r\n";
                                            if (!s) return "Out of memory";
                                            char c [25];

                                            if (trueTime && (*e).time > 1687461154) { // 2023/06/22 21:12:34, any valid time should be greater than this
                                                struct tm slt; 
                                                localtime_r (&(*e).time, &slt); 
                                                #ifndef __LOCALE_HPP__
                                                    strftime (c, sizeof (c), "[%Y/%m/%d %T] ", &slt);
                                                #else
                                                    Cstring<64> f = "["; f += lc_time_locale->getTimeFormat (); f += "] ";
                                                    strftime (c, sizeof (c), f, &slt);
                                                #endif
                                            } else {
                                                sprintf (c, "[%10lu] ", (*e).milliseconds);
                                            }
                                            if (!s.concat (c)) return "Out of memory";
                                            if (!s.concat ((*e).message)) return "Out of memory";
                                            if (sendString (s.c_str ()) <= 0) break;
                                            ++ e;
                                        }
                                        lastBack = &dmesgQueue.back (); // remember where we have finished this time
                                    }                  
                                }

                                return "\r"; // different than "" to let the calling function know that the command has been processed
                            }
                        #else
                            #error Telnet dmesg command is included but dmesg.hpp is not! #include <dmesg.hpp> prior to #including <telnetServer.h>
                        #endif
                    #endif

                    #if TELNET_QUIT_COMMAND == 1
                        const char *__quit__ () {
                            close ();
                            return "";  
                        }
                    #endif

                    #if TELNET_UPTIME_COMMAND == 1
                        Cstring<300> __uptime__ () {
                            Cstring<300> s;
                            char c [15];
                            time_t uptime = ntpClient_t ("").getUpTime (); // we'll read up time from ntpClient
                            if (uptime) { // if time is set
                                time_t t = time (NULL);
                                struct tm strTime;
                                localtime_r (&t, &strTime);
                                sprintf (c, "%02i:%02i:%02i", strTime.tm_hour, strTime.tm_min, strTime.tm_sec);
                                s = Cstring<300> (c) + " up ";     
                            } else { // time is not set (yet), just read how far clock has come untill now
                                s = "Up ";
                            }
                            uptime = millis () / 1000;
                            int seconds = uptime % 60; uptime /= 60;
                            int minutes = uptime % 60; uptime /= 60;
                            int hours = uptime % 24;   uptime /= 24; // uptime now holds days
                            if (uptime) s += Cstring<300> ((unsigned long) uptime) + " days, ";
                            sprintf (c, "%02i:%02i:%02i", hours, minutes, seconds);
                            s += c;
                            return s;
                        }
                    #endif




                    cstring __getDateTime__ () {
                        time_t t = time (NULL);
                        if (t < 1687461154) return "The time has not been set yet"; // 2023/06/22 21:12:34 is the time when I'm writing this code, any valid time should be greater than this
                        struct tm slt;
                        localtime_r (&t, &slt);
                        char s [80];
                        #ifndef __LOCALE_HPP__
                            strftime (s, sizeof (s), "[%Y/%m/%d %T]", &slt);
                        #else
                            strftime (s, sizeof (s), lc_time_locale->getTimeFormat (), &slt);
                        #endif
                        return s;
                    }

                    cstring __setDateTime__ (char *dt) {
                        struct tm slt;
                        #ifndef __LOCALE_HPP__
                            if (strptime (dt, "[%Y/%m/%d %T]", &slt) != NULL)
                        #else
                            if (strptime (dt, lc_time_locale->getTimeFormat (), &slt) != NULL)
                        #endif
                            {
                                time_t t = mktime (&slt);
                                if (t != -1) {
                                    #ifdef __TIME_FUNCTIONS__
                                        setTimeOfDay (t);
                                    #else
                                        // repeat most of the things what setTimeOfDay does (except setting __startupTime__) here
                                        struct timeval newTime = {t, 0};
                                        settimeofday (&newTime, NULL); 
                                    #endif
                                    return __getDateTime__ ();          
                                }
                            }
                        return "Wrong format of date/time specified";
                    }         


                    #ifdef __TIME_FUNCTIONS__


                        cstring __ntpdate__ (char *ntpServer = NULL) {
                            const char *e;
                            if (ntpServer)
                                e = ntpDate (ntpServer);
                            else
                                e = ntpDate ();
                            if (*e) // error
                                return e;

                            return cstring ("Got time ") + cstring (getTime ());
                        }

                        const char *__cronTab__ () {
                            cronTab.lock ();
                                if (!cronTab.size ()) {
                                    sendString ("Crontab is empty");
                                } else {
                                    for (int i = 0; i < cronTab.size (); i ++) {
                                        cstring s;
                                        char c [27];
                                        if (cronTab [i].second == ANY)      s += " * "; else { sprintf (c, "%2i ", cronTab [i].second);      s += c; }
                                        if (cronTab [i].minute == ANY)      s += " * "; else { sprintf (c, "%2i ", cronTab [i].minute);      s += c; }
                                        if (cronTab [i].hour == ANY)        s += " * "; else { sprintf (c, "%2i ", cronTab [i].hour);        s += c; }
                                        if (cronTab [i].day == ANY)         s += " * "; else { sprintf (c, "%2i ", cronTab [i].day);         s += c; }
                                        if (cronTab [i].month == ANY)       s += " * "; else { sprintf (c, "%2i ", cronTab [i].month);       s += c; }
                                        if (cronTab [i].day_of_week == ANY) s += " * "; else { sprintf (c, "%2i ", cronTab [i].day_of_week); s += c; }
                                        if (cronTab [i].lastExecuted) {
                                                                                  // ascTime (localTime (__cronEntry__ [i].lastExecuted), c, sizeof (c));
                                                                                  s += " "; s += cronTab [i].lastExecuted; s += " "; 
                                                                            } else { 
                                                                                  s += " (not executed yet)  "; 
                                                                            }
                                        if (cronTab [i].readFromFile)       s += " from /etc/crontab  "; else s += " entered from code  ";
                                        s += cronTab [i].cronCommand;
                                                                                  s += "\r\n";
                                        if (sendString (s.c_str ()) <= 0) break;
                                    }
                                }
                            cronTab.unlock ();
                            return "\r"; // different than "" to let the calling function know that the command has been processed
                        }

                    #endif

                    #if TELNET_PING_COMMAND == 1
                            const char *__ping__ (const char *pingTarget) {

                            // overload onReceive member function to get notified about intermediate results
                            class TelnetPing_t : public ThreadSafePing_t {
                                public:
                                    telnetConnection_t *tcn;

                                    void onReceive (int bytes) {
                                        // display intermediate results
                                        char buf [100];
                                        if (elapsed_time ())
                                            sprintf (buf, "\r\nReply from %s: bytes = %i time = %.3f ms", target (), size (), elapsed_time ());
                                        else
                                            sprintf (buf, "\r\nReply from %s: timeout", target ());
                                        if (tcn->sendString (buf) <= 0) stop ();
                                        return;
                                    }

                                    void onWait () {
                                        // stop waiting if a key is pressed
                                        if (tcn->peekChar ()) {
                                            tcn->recvChar ();
                                            stop ();
                                        } 
                                        return;
                                    }
                            };

                            TelnetPing_t tping;
                            tping.tcn = this;
                            tping.ping (pingTarget, PING_DEFAULT_COUNT, PING_DEFAULT_INTERVAL, PING_DEFAULT_SIZE, PING_DEFAULT_TIMEOUT);
                            char buf [200];
                            if (tping.errText ()) {
                                sprintf (buf, "\r\nError %s", tping.errText ());
                            } else {
                                sprintf (buf, "Ping statistics for %s:\r\n"
                                            "    Packets: Sent = %i, Received = %i, Lost = %i", tping.target (), tping.sent (), tping.received (), tping.lost ());
                                if (tping.sent ()) {
                                    sprintf (buf, " (%.2f%% loss)\r\nRound trip:\r\n"
                                                "   Min = %.3f ms, Max = %.3f ms, Avg = %.3f ms, Stdev = %.3f ms", (float) tping.lost () / (float) tping.sent () * 100, tping.min_time (), tping.max_time (), tping.mean_time (), sqrt (tping.var_time () / tping.received ()));
                                }
                            }
                            sendString (buf);
                            return "\r\n";
                        }
                    #endif

                    // converts binary MAC address to text
                    Cstring<17> __mac_ntos__ (byte *mac, byte macLength = 6) {
                        Cstring<17> s;
                        char c [3];
                        for (byte i = 0; i < macLength; i++) {
                            sprintf (c, "%02x", *(mac ++));
                            s += c;
                            if (i < macLength - 1) 
                                s += ":";
                        }
                        return s;
                    }

                    #if TELNET_IFCONFIG_COMMAND == 1                    
                        const char * __ifconfig__ () {
                            Cstring<600> buf;

                            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                struct netif *netif;
                                for (netif = netif_list; netif; netif = netif->next) {
                                    if (netif_is_up (netif))
                                        buf += "\r\n\n";

                                    buf += netif->name [0];
                                    buf += netif->name [1];
                                    buf += netif->num;
                                    buf += + "     hostname: ";
                                    buf += netif->hostname; // no need to check if NULL, Cstring will do this itself
                                    buf += "\r\n        hwaddr: ";
                                    buf += __mac_ntos__ (netif->hwaddr, netif->hwaddr_len);
                                    // IPv4 address
                                    if (!ip4_addr_isany_val (netif->ip_addr.u_addr.ip4)) {
                                        buf += "\r\n        ipv4 addr: ";
                                        char ip4_str [INET_ADDRSTRLEN];
                                        inet_ntop (AF_INET, &netif->ip_addr, ip4_str, sizeof (ip4_str));
                                        buf += ip4_str;
                                    }
                                    // all IPv6 addresses
                                    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                                        if (!ip6_addr_isany (&netif->ip6_addr [i].u_addr.ip6)) {
                                            buf += "\r\n        ipv6 addr: ";
                                            char ip6_str [INET6_ADDRSTRLEN];
                                            inet_ntop (AF_INET6, &netif->ip6_addr [i], ip6_str, sizeof (ip6_str));
                                            buf += ip6_str;
                                        }
                                    }
                                    buf += "\r\n        mtu: ";
                                    buf += netif->mtu;
                                } // for
                            xSemaphoreGive (getLwIpMutex ());

                            sendString (buf);
                            return "\r";
                        }
                    #endif

                    #if TELNET_IW_COMMAND == 1      
                        const char *__iw__ () {
                            Cstring<2048> buf;

                            struct netif *netif;
                            for (netif = netif_list; netif; netif = netif->next) {
                                if (netif_is_up (netif))
                                    buf += "\r\n\n";

                                buf += netif->name [0];
                                buf += netif->name [1];
                                buf += netif->num;
                                buf += + "     hostname: ";
                                buf += netif->hostname; // no need to check if NULL, string will do this itself
                                buf += "\r\n        hwaddr: ";
                                buf += __mac_ntos__ (netif->hwaddr, netif->hwaddr_len);
                                // IPv4 address
                                if (!ip4_addr_isany_val (netif->ip_addr.u_addr.ip4)) {
                                    buf += "\r\n        ipv4 addr: ";
                                    char ip4_str [INET_ADDRSTRLEN];
                                    inet_ntop (AF_INET, &netif->ip_addr, ip4_str, sizeof (ip4_str));
                                    buf += ip4_str;
                                }
                                // all IPv6 addresses
                                for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                                    if (!ip6_addr_isany (&netif->ip6_addr [i].u_addr.ip6)) {
                                        buf += "\r\n        ipv6 addr: ";
                                        char ip6_str [INET6_ADDRSTRLEN];
                                        inet_ntop (AF_INET6, &netif->ip6_addr [i], ip6_str, sizeof (ip6_str));
                                        buf += ip6_str;
                                    }
                                }

                                // STAtion
                                if (netif->name [0] == 's' && netif->name [1] == 't') {

                                    if (WiFi.status () == WL_CONNECTED) {
                                        int rssi = WiFi.RSSI ();
                                        const char *rssiDescription = ""; if (rssi == 0) rssiDescription = "not available"; else if (rssi >= -30) rssiDescription = "excelent"; else if (rssi >= -67) rssiDescription = "very good"; else if (rssi >= -70) rssiDescription = "okay"; else if (rssi >= -80) rssiDescription = "not good"; else if (rssi >= -90) rssiDescription = "bad"; else rssiDescription = "unusable";
                                        buf += "\r\n           STAtion is connected to router:\r\n"
                                               "\r\n              ipv4 addr: ";
                                        buf += WiFi.gatewayIP ().toString ().c_str ();
                                        // it would be nice to add IPv6 router's address here as well but it may be a challange getting it in Arduino
                                        buf += "     RSSI: ";
                                        buf += rssi;
                                        buf += " dBm (";
                                        buf += rssiDescription;
                                        buf += ")";
                                    } else {
                                        buf += "\r\n           STAtion is not connected to router\r\n";
                                    }

                                // local loopback
                                } else if (netif->name [0] == 'l' && netif->name [1] == 'o') {
                                    buf += "\r\n           local loopback";

                                // Access Point
                                } else if (netif->name [0] == 'a' && netif->name [1] == 'p') {
                                    wifi_sta_list_t wifi_sta_list = {};
                                    esp_wifi_ap_get_sta_list (&wifi_sta_list);
                                    if (wifi_sta_list.num) {
                                        buf += "\r\n           stations connected to Access Point (";
                                        buf += wifi_sta_list.num;
                                        buf += "):\r\n";
                                        for (int i = 0; i < wifi_sta_list.num; i++) {
                                            // it would be nice to add connected station's IPv4 and IPv6 address here as well but it may be a challange getting it in Arduino
                                            buf += "\r\n              hwaddr: ";
                                            buf += __mac_ntos__ ((byte *) wifi_sta_list.sta [i].mac);
                                            int rssi = wifi_sta_list.sta [i].rssi;
                                            const char *rssiDescription = ""; if (rssi == 0) rssiDescription = "not available"; else if (rssi >= -30) rssiDescription = "excelent"; else if (rssi >= -67) rssiDescription = "very good"; else if (rssi >= -70) rssiDescription = "okay"; else if (rssi >= -80) rssiDescription = "not good"; else if (rssi >= -90) rssiDescription = "bad"; else rssiDescription = "unusable";
                                            buf += "     RSSI: ";
                                            buf += rssi;
                                            buf += " dBm (";
                                            buf += rssiDescription;
                                            buf += ")";
                                        }

                                    } else {
                                        buf += "\r\n           there are no stations connected to Access Point\r\n";
                                    }

                                }

                            } // for

                            sendString (buf);
                            return "\r";
                        }
                    #endif

                    #if TELNET_NETSTAT_COMMAND == 1
                        const char *__netstat__ (unsigned long delaySeconds) {
                            // variables for delta calculation
                            networkTraffic_t netTraff = networkTraffic ();
                            networkTraffic_t lastNetTraff = {};

                            char buf [300]; 
                            do {
                                // clear screen
                                if (delaySeconds) 
                                    if (sendString ("\x1b[2J") <= 0) 
                                        return "\r";

                                // display totals
                                sprintf (buf, "total bytes received and sent: %72lu %9lu\r\n", netTraff.bytesReceived - lastNetTraff.bytesReceived, netTraff.bytesSent - lastNetTraff.bytesSent);

                                if (sendString (buf) <= 0) 
                                    return "\r";

                                // display header
                                sprintf (buf, "\r\n"
                                            "sck local address                           port remote address                          port  received      sent\r\n"
                                            "-----------------------------------------------------------------------------------------------------------------");
                                if (sendString (buf) <= 0) 
                                    return "\r";

                                // scan through sockets
                                for (int sockfd = LWIP_SOCKET_OFFSET; sockfd < LWIP_SOCKET_OFFSET + MEMP_NUM_NETCONN; sockfd ++) { // socket numbers are within this range

                                    // get socket type
                                    int type;
                                    socklen_t length = sizeof (type);
                                    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                        int i = getsockopt (sockfd, SOL_SOCKET, SO_TYPE, &type, &length);
                                    xSemaphoreGive (getLwIpMutex ());
                                    if (i == -1 || type != SOCK_STREAM)
                                        continue; // skip SOCK_DGRAM socket

                                    // get server's IP address
                                    char thisIP [INET6_ADDRSTRLEN] = ""; 
                                    int thisPort = 0;
                                    char remoteIP [INET6_ADDRSTRLEN] = "";
                                    int remotePort = 0;

                                    struct sockaddr_storage addr = {}; 
                                    socklen_t len = sizeof (addr);
                                    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                        i = getsockname (sockfd, (struct sockaddr *) &addr, &len);
                                    xSemaphoreGive (getLwIpMutex ());
                                    if (i != -1) {
                                        struct sockaddr_in6* s = (struct sockaddr_in6 *) &addr; 
                                        inet_ntop (AF_INET6, &s->sin6_addr, thisIP, sizeof (thisIP));
                                        // this works fine when the connection is IPv6, but when it is a 
                                        // IPv4 connection we are getting IPv6 representation of IPv4 address like "::FFFF:10.18.1.140"
                                        if (strstr (thisIP, ".")) {
                                            char *p = thisIP;
                                            for (char *q = p; *q; q++)
                                                if (*q == ':')
                                                    p = q;
                                            if (p != thisIP)
                                                strcpy (thisIP, p + 1);
                                        }
                                        thisPort = ntohs (s->sin6_port);

                                        // get client's IP address
                                        addr = {}; 
                                        // len = sizeof (addr);
                                        xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                            i = getpeername (sockfd, (struct sockaddr *) &addr, &len);
                                        xSemaphoreGive (getLwIpMutex ());
                                        if (i != -1) {
                                            struct sockaddr_in6* s = (struct sockaddr_in6 *) &addr; 
                                            inet_ntop (AF_INET6, &s->sin6_addr, remoteIP, sizeof (remoteIP));
                                            // this works fine when the connection is IPv6, but when it is a 
                                            // IPv4 connection we are getting something like "::FFFF:10.18.1.140"
                                            if (strstr (remoteIP, ".")) {
                                                char *p = remoteIP;
                                                for (char *q = p; *q; q++)
                                                    if (*q == ':')
                                                        p = q;
                                                if (p != remoteIP)
                                                    strcpy (remoteIP, p + 1);
                                            }
                                            remotePort = ntohs (s->sin6_port);
                                        } 

                                        if (*thisIP && *remoteIP) {
                                            sprintf (buf, "\r\n %2i %-39s%5i %-39s%5i %9lu %9lu", sockfd, thisIP, thisPort, remoteIP, remotePort, netTraff [sockfd].bytesReceived - lastNetTraff [sockfd].bytesReceived, netTraff [sockfd].bytesSent - lastNetTraff [sockfd].bytesSent);
                                            if (sendString (buf) <= 0) 
                                                return "\r";

                                            // update variables for delta calculation
                                            lastNetTraff = netTraff;
                                            netTraff = networkTraffic ();
                                        }
                                        
                                    } // getsockname
                                } // for

                                // wait for a key press
                                unsigned long startMillis = millis ();
                                while (millis () - startMillis < (delaySeconds * 1000)) {
                                    delay (100);
                                    if (peekChar ()) {
                                        recvChar (); // read pending character
                                        return "\r"; // return if user pressed Ctrl-C or any key
                                    } 
                                }

                            } while (delaySeconds);
                            return "\r"; // different than "" to let the calling function know that the command has been processed
                        }
                    #endif

                    #if TELNET_KILL_COMMAND == 1
                        Cstring<300> __kill__ (int sockfd) {
                            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                int i = ::close (sockfd);
                            xSemaphoreGive (getLwIpMutex ());
                            if (i < 0) {
                                dmesgQueue << "[telnetConn] close error: " << errno << " " << strerror (errno);
                                return Cstring<300> ("Error: ") + Cstring<300> (errno) + " " + strerror (errno);
                            }
                            return "Socked closed";
                        }
                    #endif

                    #if TELNET_CURL_COMMAND == 1
                        const char *__curl__ (const char *method, char *url) {
                            char server [65];
                            char addr [128] = "/";
                            int port = 80;
                            if (sscanf (url, "http://%64[^:]:%i/%126s", server, &port, addr + 1) < 2) { // we haven't got at least server, port and the default address
                                if (sscanf (url, "http://%64[^/]/%126s", server, addr + 1) < 1) { // we haven't got at least server and the default address  
                                    return "Wrong url, use form of http://server/address or http://server:port/address";
                                }
                            }
                            if (port <= 0) 
                                return "Wrong port number";
                            if (server [0] == '[' && server [strlen (server) - 1] == ']') {
                                server [strlen (server) - 1] = 0;
                                strcpy (server, server + 1);
                            }
                            String r = httpRequest (server, port, addr, method);
                            if (!r) 
                                return "Out of memory";
                            sendString (r.c_str ());
                            return "\r"; // different than "" to let the calling function know that the command has been processed
                        }
                    #endif


                    #if TELNET_LS_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__
                            const char *__ls__ (char *directoryName) {
                                if (!__fileSystem__.mounted ())                                                         return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath = __fileSystem__.makeFullPath (directoryName, __workingDirectory__);
                                if (!__fileSystem__.isDirectory (fullPath))                                             return "Invalid directory name";
                                if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__))        return "Access denyed";

                                // display information about files and subdirectories
                                bool firstRecord = true;
                                threadSafeFS::File d = __fileSystem__.open (fullPath); 
                                if (!d)                                                 
                                    return "Out of resources";
                                for (threadSafeFS::File f = d.openNextFile (); f; f = d.openNextFile ()) {
                                    Cstring<255> fullFileName = fullPath;
                                    if (fullFileName [fullFileName.length () - 1] != '/') fullFileName += '/'; fullFileName += f.name ();
                                    if (sendString (firstRecord ? __fileSystem__.fileInformation (fullFileName) : Cstring<300> ("\r\n") + __fileSystem__.fileInformation (fullFileName)) <= 0) { 
                                        d.close (); 
                                        return "\r"; 
                                    }
                                    firstRecord = false;
                                }
                                d.close ();
                                return "\r"; // different than "" to let the calling function know that the command has been processed
                            }
                        #else
                            #error Telnet ls command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif
                    #endif

                    #if TELNET_TREE_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__
                            // iteratively scan directory tree, recursion takes too much stack
                            const char *__tree__ (char *directoryName) {
                                if (!__fileSystem__.mounted ())                                                         return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath = __fileSystem__.makeFullPath (directoryName, __workingDirectory__);
                                if (!__fileSystem__.isDirectory (fullPath))                                             return "Invalid directory name";
                                if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__))        return "Access denyed";

                                // keep directory names saved in vector for later recursion
                                queue<Cstring<255>> dirList;
                                if (dirList.push (fullPath))
                                    return "Out of memory";
                                bool firstRecord = true;
                                while (dirList.size () != 0) {

                                    // 1. take the first directory from dirList
                                    fullPath = dirList [0];
                                    dirList.pop (); // dirList.erase (dirList.begin ());

                                    // 2. display directory info
                                    if (sendString (firstRecord ? __fileSystem__.fileInformation (fullPath, true) : Cstring<300> ("\r\n") + __fileSystem__.fileInformation (fullPath, true)) <= 0) 
                                        return "Out of memory";
                                    firstRecord = false;

                                    // 3. display information about files and remember subdirectories in dirList
                                    File d = __fileSystem__.open (fullPath); 
                                    if (!d) return "Out of resources";
                                    for (threadSafeFS::File f = d.openNextFile (); f; f = d.openNextFile ()) {
                                        Cstring<255> directoryPath = fullPath; 
                                        if (directoryPath [directoryPath.length () - 1] != '/') 
                                            directoryPath += '/'; 
                                        directoryPath += f.name ();                       
                                        if (f.isDirectory ()) {
                                            // save directory name for later recursion
                                            if (dirList.push (directoryPath)) { d.close (); return "Out of memory"; }
                                        } else {
                                            // output file information
                                            if (sendString (Cstring<300> ("\r\n") + __fileSystem__.fileInformation (directoryPath, true)) <= 0) { d.close (); return "Out of memory"; }
                                        } 
                                    }
                                    d.close ();
                                }
                                return "\r"; // different than "" to let the calling function know that the command has been processed
                            }
                        #else
                            #error Telnet tree command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif
                    #endif

                    #if TELNET_MKDIR_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__
                            Cstring<300> __mkdir__ (char *directoryName) { 
                                if (!__fileSystem__.mounted ())                                                         return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath = __fileSystem__.makeFullPath (directoryName, __workingDirectory__);
                                if (fullPath == "")                                                                     return "Invalid directory name";
                                if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__))        return "Access denyed";
                                if (__fileSystem__.mkdir (fullPath))                                                    return fullPath + " made";
                                                                                                                        return Cstring<300> ("Can't make ") + fullPath;
                            }
                        #else
                            #error Telnet mkdir command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif                    
                    #endif

                    #if TELNET_RMDIR_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__
                            Cstring<300> __rmdir__ (char *directoryName) { 
                                if (!__fileSystem__.mounted ())                                                         return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath = __fileSystem__.makeFullPath (directoryName, __workingDirectory__);
                                if (fullPath == "" || !__fileSystem__.isDirectory (fullPath))                           return "Invalid directory name";
                                if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__))        return "Access denyed";
                                if (fullPath == __homeDirectory__)                                                      return "You can't remove your home directory";
                                if (fullPath == __workingDirectory__)                                                   return "You can't remove your working directory";
                        
                                if (__fileSystem__.rmdir (fullPath))                                                    return fullPath + " removed";
                                                                                                                        return Cstring<300> ("Can't remove ") + fullPath;
                            }
                        #else
                            #error Telnet rmdir command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif                    
                    #endif

                    #if TELNET_CD_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__
                            Cstring<300> __cd__ (const char *directoryName) { 
                                if (!__fileSystem__.mounted ())                                                         return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath = __fileSystem__.makeFullPath (directoryName, __workingDirectory__);
                                if (fullPath == "" || !__fileSystem__.isDirectory (fullPath))                           return "Invalid directory name";
                                if (!__fileSystem__.userHasRightToAccessDirectory (fullPath, __homeDirectory__))        return "Access denyed";
                                strcpy (__workingDirectory__, fullPath);                                                return Cstring<300> ("Your working directory is ") + fullPath;
                            }
                        #else
                            #error Telnet cd command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif                           
                    #endif

                    #if TELNET_PWD_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__
                            Cstring<300> __pwd__ () { 
                                if (!__fileSystem__.mounted ())                                                         return "File system not mounted. You may have to format flash disk first";
                                // remove extra /
                                Cstring<255> s = __workingDirectory__;
                                if (s [s.length () - 1] == '/') s [s.length () - 1] = 0;
                                if (s == "") s = "/"; 
                                                                                                                        return Cstring<300> ("Your working directory is ") + s;
                            }
                        #else
                            #error Telnet pwd command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif                           
                    #endif

                    #if TELNET_CAT_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__

                            Cstring<300> __catFileToClient__ (char *fileName) {
                                if (!__fileSystem__.mounted ())                                                             return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath = __fileSystem__.makeFullPath (fileName, __workingDirectory__);
                                if (fullPath == "" || !__fileSystem__.isFile (fullPath))                                    return "Invalid file name";
                                if (!__fileSystem__.userHasRightToAccessFile (fullPath, __homeDirectory__))                 return "Access denyed";

                                char buff [1440];
                                *buff = 0;
                                
                                threadSafeFS::File f = __fileSystem__.open (fullPath, FILE_READ); 
                                if (f) {
                                    int i = strlen (buff);
                                    while (f.available ()) {
                                        switch (*(buff + i) = f.read ()) {
                                            case '\r':  // ignore
                                                        break;
                                            case '\n':  // LF-CRLF conversion
                                                        *(buff + i ++) = '\r'; 
                                                        *(buff + i ++) = '\n';
                                                        break;
                                            default:
                                                        i++;                  
                                        }
                                        if (i >= sizeof (buff) - 2) { 
                                            if (sendBlock (buff, i) <= 0) { 
                                                f.close (); 
                                                return "\r"; 
                                            }
                                            i = 0; 
                                        }
                                    }
                                    if (i) { 
                                        if (sendBlock (buff, i) <= 0) { 
                                            f.close (); 
                                            return "\r"; 
                                        }
                                    }
                                } else {
                                    f.close (); 
                                    return Cstring<300> ("Can't read ") + fullPath;
                                }
                                f.close ();
                                return "\r"; // different than "" to let the calling function know that the command has been processed
                            }

                            Cstring<300> __catClientToFile__ (char *fileName) {
                                if (!__fileSystem__.mounted ())                                                             return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath = __fileSystem__.makeFullPath (fileName, __workingDirectory__);
                                if (fullPath == "" || __fileSystem__.isDirectory (fullPath))                                return "Invalid file name";
                                if (!__fileSystem__.userHasRightToAccessFile (fullPath, __homeDirectory__))                 return "Access denyed";

                                threadSafeFS::File f = __fileSystem__.open (fullPath, FILE_WRITE);
                                if (f) {
                                    while (char c = recvChar ()) { 
                                        switch (c) {
                                            case 0:     // Error
                                            case 3:     // Ctrl-C
                                                        f.close (); 
                                                        return fullPath + " not fully written";
                                            case 4:     // Ctrl-D or Ctrl-Z
                                                        f.close (); 
                                                        return Cstring<300> ("\r\n") + fullPath + " written";
                                            case 10:    // ignore
                                                        break;
                                            case 13:    // CR -> CRLF
                                                        if (f.write ((uint8_t *) "\r\n", 2) != 2) { 
                                                            f.close (); 
                                                            return Cstring<300> ("Can't write ") + fullPath;
                                                        } 
                                                        // echo
                                                        if (sendString ("\r\n") <= 0) return "\r";
                                                        break;
                                            default:    // character 
                                                        if (f.write ((uint8_t *) &c, 1) != 1) { 
                                                            f.close (); 
                                                            return Cstring<300> ("Can't write ") + fullPath;
                                                        } 
                                                        // echo
                                                        if (sendBlock (&c, 1) <= 0) return "\r";
                                                        break;
                                        }
                                    }
                                    f.close ();
                                } else {
                                    return Cstring<300> ("Can't write ") + fullPath;
                                }
                                return "\r";
                            }

                        #else
                            #error Telnet cat command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif    
                    #endif

                    #if TELNET_VI_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__
                            // not really vi but small and simple text editor
                            Cstring<300> __vi__ (char *fileName) {
                                // a good reference for telnet ESC codes: https://en.wikipedia.org/wiki/ANSI_escape_code
                                // and: https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
                                if (!__fileSystem__.mounted ())                                                             return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath = __fileSystem__.makeFullPath (fileName, __workingDirectory__);
                                if (fullPath == "")                                                                         return "Invalid file name";
                                if (!__fileSystem__.userHasRightToAccessFile (fullPath, __homeDirectory__))                 return "Access denyed";
                        
                                // 1. create a new file if it doesn't exist
                                if (!__fileSystem__.isFile (fullPath)) {
                                    threadSafeFS::File f = __fileSystem__.open (fullPath, FILE_WRITE);
                                    if (f.isDirectory ()) { f.close ();                                                     return "Can't edit directory"; }
                                    if (f) { f.close (); if (sendString ("\r\nFile created") <= 0) return "\r"; }
                                    else                                                                                    return Cstring<300> ("Can't create ") + fullPath;
                                }
                        
                                // 2. read the file content into internal vi data structure (lines of Strings)
                                #define MAX_LINES 9999 // there are 4 places reserved for line numbers on client screen, so MAX_LINES can go up to 9999
                                #define LEAVE_FREE_HEAP 6 * 1024 // always keep at least 6 KB free to other processes so vi command doesn't block everything else from working
                                vector<String> lines; // vector of Arduino Strings
                                // lines.reserve (1000);
                                bool dirty = false;
                                {
                                    threadSafeFS::File f = __fileSystem__.open (fullPath, FILE_READ);
                                    if (f) {
                                        if (!f.isDirectory ()) {
                                            while (f.available ()) { 
                                                if (ESP.getFreeHeap () < LEAVE_FREE_HEAP) { // always leave at least 32 KB free for other things that may be running on ESP32
                                                    f.close ();                                                             return "Out of memory"; 
                                                }
                                                char c = (char) f.read ();
                                                switch (c) {
                                                    case '\r':  break; // ignore
                                                    case '\n':  if (ESP.getFreeHeap () < LEAVE_FREE_HEAP || lines.push_back ("")) { // always leave at least 32 KB free for other things that may be running on ESP32
                                                                    f.close ();                                             return "Out of memory";
                                                                }
                                                                if (lines.size () >= MAX_LINES) { 
                                                                    f.close ();                                             return fullPath + " has too many lines vi"; 
                                                                }
                                                                break; 
                                                    case '\t':  if (!lines.size ()) 
                                                                    if (lines.push_back ("")) {
                                                                        f.close ();                                         return "Out of memory"; // vi editor (the code below) needs at least 1 (empty) line where text can be entered
                                                                    }
                                                                if (!lines [lines.size () - 1].concat ("    ")) {
                                                                    f.close ();
                                                                    return "Out of memory"; // treat tab as 4 spaces, check success of concatenation
                                                                }
                                                                break;
                                                    default:    if (!lines.size ()) 
                                                                    if (lines.push_back ("")) {
                                                                        f.close ();                                         return "Out of memory"; // vi editor (the code below) needs at least 1 (empty) line where text can be entered
                                                                    }
                                                                if (!lines [lines.size () - 1].concat (c)) {
                                                                    f.close ();                                             return "Out of memory"; // check success of concatenation
                                                                }
                                                                break;
                                                }
                                            }
                                            f.close ();
                                        } else {
                                            f.close ();                                                                     return "Can't edit directory";
                                        }
                                    } else {
                                        return Cstring<300> ("Can't read ") + fullPath;
                                    }
                                }    
                                if (!lines.size ()) if (lines.push_back (""))                                               return "Out of memory"; // vi editor (the code below) needs at least 1 (empty) line where text can be entered

                                // 3. discard any already pending characters from client
                                while (peekChar ()) 
                                    recvChar ();
                                // 4. get information about client window size
                                if (__clientWindowWidth__) { // we know that client reports its window size, ask for latest information (the user might have resized the window since beginning of telnet session)
                                    if (sendString ((char *) IAC DO NAWS) <= 0) return "\r";
                                    // client will reply in the form of: IAC (255) SB (250) NAWS (31) col1 col2 row1 row1 IAC (255) SE (240) but this will be handeled internally by readTelnet function
                                } else { // just assume the defaults and hope that the result will be OK
                                __clientWindowWidth__ = 80; 
                                __clientWindowHeight__ = 24;   
                                }                
                                // wait for client's response
                                {
                                    char c;
                                    while (peek (&c, 1) == 0)
                                        delay (10);
                                }
                                if (__clientWindowWidth__ < 44 || __clientWindowHeight__ < 5)                               return "Clinent telnet window is too small for vi";

                                // 5. edit 
                                int textCursorX = 0;  // vertical position of cursor in text
                                int textCursorY = 0;  // horizontal position of cursor in text
                                int textScrollX = 0;  // vertical text scroll offset
                                int textScrollY = 0;  // horizontal text scroll offset
                        
                                bool redrawHeader = true;
                                bool redrawAllLines = true;
                                bool redrawLineAtCursor = false; 
                                bool redrawFooter = true;
                                Cstring<300> message = Cstring<300> (" ") + Cstring<64> (lines.size ()) + " lines ";
                        
                                                        // clear screen
                                                        if (sendString ("\x1b[2J") <= 0) return "\r";  // ESC[2J = clear screen
                        
                                while (true) {
                                    // a. redraw screen 
                                    Cstring<300> s;
                                
                                    if (redrawHeader)   { 
                                                            s = "\x1b[H----+"; s.rPad (__clientWindowWidth__ - 26, '-'); s += " Save: Ctrl-S, Exit: Ctrl-X -"; // ESC[H = move cursor home
                                                            if (sendString (s) <= 0) return "\r";  
                                                            redrawHeader = false;
                                                        }
                                    if (redrawAllLines) {
                            
                                                            // Redrawing algorithm: straight forward idea is to scan screen lines with text i ∈ [2 .. __clientWindowHeight__ - 1], calculate text line on
                                                            // this possition: i - 2 + textScrollY and draw visible part of it: line [...].substring (textScrollX, __clientWindowWidth__ - 5 + textScrollX), __clientWindowWidth__ - 5).
                                                            // When there are frequent redraws the user experience is not so good since we often do not have enough time to redraw the whole screen
                                                            // between two key strokes. Therefore we'll always redraw just the line at cursor position: textCursorY + 2 - textScrollY and then
                                                            // alternate around this line until we finish or there is another key waiting to be read - this would interrupt the algorithm and
                                                            // redrawing will repeat after ne next character is processed.
                                                            {
                                                                int nextScreenLine = textCursorY + 2 - textScrollY;
                                                                int nextTextLine = nextScreenLine - 2 + textScrollY;
                                                                bool topReached = false;
                                                                bool bottomReached = false;
                                                                for (int i = 0; (!topReached || !bottomReached) && !(peekChar ()) ; i++) { // if user presses any key redrawing stops (bettwr user experienca while scrolling)
                                                                    if (i % 2 == 0) { nextScreenLine -= i; nextTextLine -= i; } else { nextScreenLine += i; nextTextLine += i; }
                                                                    if (nextScreenLine == 2) topReached = true;
                                                                    if (nextScreenLine == __clientWindowHeight__ - 1) bottomReached = true;
                                                                    if (nextScreenLine > 1 && nextScreenLine < __clientWindowHeight__) {
                                                                        // draw nextTextLine at nextScreenLine 
                                                                        if (nextTextLine < lines.size ())
                                                                            snprintf (s, s.max_size (), "\x1b[%i;0H%4i|", nextScreenLine, nextTextLine + 1);  // display line number - users would count lines from 1 on, whereas program counts them from 0 on
                                                                        else
                                                                            snprintf (s, s.max_size (), "\x1b[%i;0H    |", nextScreenLine);  // line numbers will not be displayed so the user will know where the file ends                                        
                                                                        // ESC[line;columnH = move cursor to line;column, it is much faster to append line with spaces then sending \x1b[0J (delte from cursor to the end of screen)
                                                                        size_t escCommandLength = s.length ();
                                                                        if (nextTextLine < lines.size ())
                                                                            if (lines [nextTextLine].length () >= textScrollX)                                        
                                                                            s += lines [nextTextLine].c_str () + textScrollX;
                                                                        s.rPad (__clientWindowWidth__ + escCommandLength, ' ');
                                                                        s.erase (__clientWindowWidth__ - 5 + escCommandLength);
                                                                        if (sendString (s) <= 0) return "\r";
                                                                    }
                                                                }
                                                                if (topReached && bottomReached) redrawAllLines = false; // if we have drown all the lines we don't have to run this code again 
                                                            }
                                                            
                                    } else if (redrawLineAtCursor) {
                                                            // calculate screen line from text cursor position
                                                            // ESC[line;columnH = move cursor to line;column (columns go from 1 to clientWindowsColumns - inclusive), it is much faster to append line with spaces then sending \x1b[0J (delte from cursor to the end of screen)
                                                            snprintf (s, s.max_size (), "\x1b[%i;6H", textCursorY + 2 - textScrollY);
                                                            size_t escCommandLength = s.length ();
                                                            if (lines [textCursorY].length () >= textScrollX)
                                                            s += lines [textCursorY].c_str () + textScrollX;
                                                            s.rPad (__clientWindowWidth__ + escCommandLength, ' ');
                                                            s.erase (__clientWindowWidth__ - 5 + escCommandLength);
                                                            if (sendString (s) <= 0) return "\r"; 
                                                            redrawLineAtCursor = false;
                                                        }
                                    if (redrawFooter)   {                                  
                                                            snprintf (s, s.max_size (), "\x1b[%i;0H----+", __clientWindowHeight__);
                                                            s.rPad (__clientWindowWidth__ + s.length () - 5, '-');                                  
                                                            if (sendString (s) <= 0) return "\r";
                                                            redrawFooter = false;
                                                        }
                                    if (message != "")  {
                                                            snprintf (s, s.max_size (), "\x1b[%i;2H%s", __clientWindowHeight__, message);
                                                            if (sendString (s) <= 0) return "\r"; 
                                                            message = ""; redrawFooter = true; // we'll clear the message the next time screen redraws
                                                        }

                                    // b. restore cursor position - calculate screen coordinates from text coordinates
                                    {
                                        // ESC[line;columnH = move cursor to line;column
                                        snprintf (s, s.max_size (), "\x1b[%i;%iH", textCursorY - textScrollY + 2, textCursorX - textScrollX + 6);
                                        if (sendString (s) <= 0) return "\r"; // ESC[line;columnH = move cursor to line;column
                                    }
                            
                                    // c. read and process incoming stream of characters
                                    char c = 0;
                                    delay (1);
                                    if (!(c = recvChar ())) return "\r";
                                    switch (c) {
                                        case 24:  // Ctrl-X
                                                if (dirty) {
                                                    Cstring<64> tmp = Cstring<64> ("\x1b[") + Cstring<64> (__clientWindowHeight__) + ";2H Save changes (y/n)? ";
                                                    if (sendString (tmp) <= 0) return "\r"; 
                                                    redrawFooter = true; // overwrite this question at next redraw
                                                    while (true) {                                                     
                                                        if (!(c = recvChar ())) return "\r";
                                                        if (c == 'y') goto saveChanges;
                                                        if (c == 'n') break;
                                                    }
                                                } 
                                                {
                                                    // Cstring<64> tmp = Cstring<64> ("\x1b[") + Cstring<64> (__clientWindowHeight__) + ";2H Share and Enjoy ----\r\n";
                                                    // if (sendString (tmp) <= 0) return "";
                                                }
                                                return Cstring<300> ("\x1b[") + Cstring<64> (__clientWindowHeight__) + ";2H";
                                        case 19:  // Ctrl-S
                                saveChanges:
                                                // save changes to fp
                                                {
                                                    bool e = false;
                                                        threadSafeFS::File f = __fileSystem__.open (fullPath, FILE_WRITE);
                                                        if (f) {
                                                            if (!f.isDirectory ()) {
                                                                for (int i = 0; i < lines.size (); i++) {
                                                                    int toWrite = strlen (lines [i].c_str ()); 
                                                                    if (i) 
                                                                        toWrite += 2;
                                                                    if (toWrite != f.write (i ? (uint8_t *) ("\r\n" + lines [i]).c_str (): (uint8_t *) lines [i].c_str (), toWrite)) { 
                                                                        e = true; 
                                                                        break; 
                                                                    }
                                                                }
                                                            }
                                                            f.close ();
                                                        }
                                                    if (e) { message = " Could't save changes "; } else { message = " Changes saved "; dirty = false; }
                                                }
                                                break;
                                        case 27:  // ESC [A = up arrow, ESC [B = down arrow, ESC[C = right arrow, ESC[D = left arrow, 
                                                if (!(c = recvChar ())) return "\r";
                                                switch (c) {
                                                    case '[': // ESC [
                                                            if (!(c = recvChar ())) return "\r";
                                                            switch (c) {
                                                                case 'A':   // ESC [ A = up arrow
                                                                            if (textCursorY > 0) textCursorY--; 
                                                                            if (textCursorX > lines [textCursorY].length ()) textCursorX = lines [textCursorY].length ();
                                                                            break;                          
                                                                case 'B':   // ESC [ B = down arrow
                                                                            if (textCursorY < lines.size () - 1) textCursorY++;
                                                                            if (textCursorX > lines [textCursorY].length ()) textCursorX = lines [textCursorY].length ();
                                                                            break;
                                                                case 'C':   // ESC [ C = right arrow
                                                                            if (textCursorX < lines [textCursorY].length ()) textCursorX++;
                                                                            else if (textCursorY < lines.size () - 1) { textCursorY++; textCursorX = 0; }
                                                                            break;
                                                                case 'D':   // ESC [ D = left arrow
                                                                            if (textCursorX > 0) textCursorX--;
                                                                            else if (textCursorY > 0) { textCursorY--; textCursorX = lines [textCursorY].length (); }
                                                                            break;        
                                                                case '1':   // ESC [ 1 = home
                                                                            if (!(c = recvChar ())) return "\r"; // read final '~'
                                                                            textCursorX = 0;
                                                                            break;
                                                                case 'H':   // ESC [ H = home in Linux
                                                                            textCursorX = 0;
                                                                            break;
                                                                case '4':   // ESC [ 4 = end
                                                                            if (!(c = recvChar ())) return "\r";  // read final '~'
                                                                            textCursorX = lines [textCursorY].length ();
                                                                            break;
                                                                case 'F':   // ESC [ F = end in Linux
                                                                            textCursorX = lines [textCursorY].length ();
                                                                            break;
                                                                case '5':   // ESC [ 5 = pgup
                                                                            if (!(c = recvChar ())) return "\r"; // read final '~'
                                                                            textCursorY -= (__clientWindowHeight__ - 2); if (textCursorY < 0) textCursorY = 0;
                                                                            if (textCursorX > lines [textCursorY].length ()) textCursorX = lines [textCursorY].length ();
                                                                            break;
                                                                case '6':   // ESC [ 6 = pgdn
                                                                            if (!(c = recvChar ())) return "\r"; // read final '~'
                                                                            textCursorY += (__clientWindowHeight__ - 2); if (textCursorY >= lines.size ()) textCursorY = lines.size () - 1;
                                                                            if (textCursorX > lines [textCursorY].length ()) textCursorX = lines [textCursorY].length ();
                                                                            break;  
                                                                case '3':   // ESC [ 3 
                                                                            if (!(c = recvChar ())) return "\r";
                                                                            switch (c) {
                                                                                case '~': // ESC [ 3 ~ (126) - putty reports del key as ESC [ 3 ~ (126), since it also report backspace key as del key let' treat del key as backspace                                                                 
                                                                                        #ifdef SWITHC_DEL_AND_BACKSPACE
                                                                                            goto delete_key;
                                                                                        #else
                                                                                            goto backspace_key;
                                                                                        #endif
                                                                                default:  // ignore
                                                                                        break;
                                                                            }
                                                                            break;
                                                                default:    // ignore
                                                                            break;
                                                            }
                                                            break;
                                                    default:// ignore
                                                            break;
                                                }
                                                break;
                                        case 8:   // Windows telnet.exe: back-space, putty does not report this code
                                backspace_key:
                                                if (textCursorX > 0) { // delete one character left of cursor position
                                                    int l1 = lines [textCursorY].length ();
                                                    lines [textCursorY].remove (textCursorX - 1, 1);
                                                    int l2 = lines [textCursorY].length (); if (l2 + 1 != l1) return "Out of memory"; // check success of character deletion
                                                    dirty = redrawLineAtCursor = true; // we only have to redraw this line
                                                    setIdleTimeout (0); // infinite
                                                    textCursorX--;
                                                } else if (textCursorY > 0) { // combine 2 lines
                                                    textCursorY--;
                                                    textCursorX = lines [textCursorY].length (); 
                                                    if (!lines [textCursorY].concat (lines [textCursorY + 1])) return "Out of memory"; // check success of concatenation
                                                    lines.erase (lines.begin () + textCursorY + 1);
                                                    dirty = redrawAllLines = true; // we need to redraw all visible lines (at least lines from testCursorY down but we wont write special cede for this case)
                                                    setIdleTimeout (0); // infinite
                                                }
                                                break; 
                                        case 127: // Windows telnet.exe: delete, putty, Linux: backspace
                                    delete_key:
                                                if (textCursorX < lines [textCursorY].length ()) { // delete one character at cursor position
                                                    int l1 = lines [textCursorY].length ();
                                                    lines [textCursorY].remove (textCursorX, 1);
                                                    int l2 = lines [textCursorY].length (); if (l2 + 1 != l1) return "Out of memory"; // check success of character deletion
                                                    dirty = redrawLineAtCursor = true; // we only need to redraw this line
                                                    setIdleTimeout (0); // infinite
                                                } else { // combine 2 lines
                                                    if (textCursorY < lines.size () - 1) {
                                                        if (!lines [textCursorY].concat (lines [textCursorY + 1])) return "Out of memory"; // check success of concatenation
                                                        lines.erase (lines.begin () + textCursorY + 1); 
                                                        dirty = redrawAllLines = true; // we need to redraw all visible lines (at least lines from testCursorY down but we wont write special cede for this case)                          
                                                        setIdleTimeout (0); // infinite
                                                    }
                                                }
                                                break;
                                        case 13:// enter
                                                // split current line at cursor into textCursorY + 1 and textCursorY
                                                if (lines.size () >= MAX_LINES || ESP.getFreeHeap () < LEAVE_FREE_HEAP || lines.insert (lines.begin () + textCursorY + 1, lines [textCursorY].substring (textCursorX))) {
                                                    message = " Out of memory or too many lines ";
                                                } else {
                                                    lines [textCursorY] = lines [textCursorY].substring (0, textCursorX);
                                                    if (lines [textCursorY].length () != textCursorX) return "Out of memory"; // check success of string assignment
                                                    textCursorX = 0;
                                                    dirty = redrawAllLines = true; // we need to redraw all visible lines (at least lines from testCursorY down but we wont write special cede for this case)
                                                    setIdleTimeout (0); // infinite
                                                    textCursorY++;
                                                }
                                                break;
                                        case 10:  // ignore
                                                break;
                                        default:  // normal character
                                                if (ESP.getFreeHeap () < LEAVE_FREE_HEAP) { 
                                                    message = " Out of memory ";
                                                } else {
                                                    if (c == '\t') s = "    "; else s = c; // treat tab as 4 spaces
                                                    int l1 = lines [textCursorY].length (); 
                                                    lines [textCursorY] = lines [textCursorY].substring (0, textCursorX) + s + lines [textCursorY].substring (textCursorX); // inser character into line textCurrorY at textCursorX position
                                                    int l2 = lines [textCursorY].length (); if (l2 <= l1) return "Out of memory"; // check success of insertion of characters
                                                    dirty = redrawLineAtCursor = true; // we only have to redraw this line
                                                    setIdleTimeout (0); // infinite
                                                    textCursorX += s.length ();
                                                }
                                                break;
                                    }         
                                    // if cursor has moved - should we scroll?
                                    {
                                        if (textCursorX - textScrollX >= __clientWindowWidth__ - 5) {
                                            textScrollX = textCursorX - (__clientWindowWidth__ - 5) + 1; // scroll left if the cursor fell out of right client window border
                                            redrawAllLines = true; // we need to redraw all visible lines
                                        }
                                        if (textCursorX - textScrollX < 0) {
                                            textScrollX = textCursorX; // scroll right if the cursor fell out of left client window border
                                            redrawAllLines = true; // we need to redraw all visible lines
                                        }
                                        if (textCursorY - textScrollY >= __clientWindowHeight__ - 2) {
                                            textScrollY = textCursorY - (__clientWindowHeight__ - 2) + 1; // scroll up if the cursor fell out of bottom client window border
                                            redrawAllLines = true; // we need to redraw all visible lines
                                        }
                                        if (textCursorY - textScrollY < 0) {
                                            textScrollY = textCursorY; // scroll down if the cursor fell out of top client window border
                                            redrawAllLines = true; // we need to redraw all visible lines
                                        }
                                    }
                                }
                                return "\r";
                            }
                        #else
                            #error Telnet vi command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif    
                    #endif

                    #if TELNET_CP_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__
                            Cstring<300> __cp__ (char *srcFileName, char *dstFileName) { 
                                if (!__fileSystem__.mounted ())                                                             return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath1 = __fileSystem__.makeFullPath (srcFileName, __workingDirectory__);
                                if (fullPath1 == "")                                                                        return "Invalid source file name";
                                if (!__fileSystem__.userHasRightToAccessFile (fullPath1, __homeDirectory__))                return "Access to source file denyed";
                                Cstring<255> fullPath2 = __fileSystem__.makeFullPath (dstFileName, __workingDirectory__);
                                if (fullPath2 == "")                                                                        return "Invalid destination file name";
                                if (!__fileSystem__.userHasRightToAccessFile (fullPath2, __homeDirectory__))                return "Access destination file denyed";

                                threadSafeFS::File f1 = __fileSystem__.open (fullPath1, FILE_READ);
                                if (!f1)                                                                                    return Cstring<300> ("Can't read ") + fullPath1;
                                if (f1.isDirectory ()) { f1.close ();                                                       return "Can't copy directory"; }
                                threadSafeFS::File f2 = __fileSystem__.open (fullPath2, FILE_WRITE);
                                if (!f2) { f1.close ();                                                                     return Cstring<300> ("Can't write ") + fullPath2; }
                                Cstring<300> retVal = "File copied";

                                int bytesReadTotal = 0;
                                int bytesWrittenTotal = 0;
                                char buff [1024];
                                do {
                                    int bytesReadThisTime = f1.read ((uint8_t *) buff, sizeof (buff));
                                    if (bytesReadThisTime == 0) break; // finished, success
                                    bytesReadTotal += bytesReadThisTime;

                                    int bytesWrittenThisTime = f2.write ((uint8_t *) buff, bytesReadThisTime);
                                    bytesWrittenTotal += bytesWrittenThisTime;

                                    if (bytesWrittenThisTime != bytesReadThisTime) { retVal = Cstring<300> ("Can't write ") + fullPath2; break; }
                                } while (true);
                                f2.close ();
                                f1.close ();
                                return retVal;
                            }
                        #else
                            #error Telnet cp command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif                           
                    #endif

                    #if TELNET_RM_COMMAND == 1
                        #ifdef __THREAD_SAFE_FS__
                            Cstring<300> __rm__ (char *fileName) {
                                if (!__fileSystem__.mounted ())                                                             return "File system not mounted. You may have to format flash disk first";
                                Cstring<255> fullPath = __fileSystem__.makeFullPath (fileName, __workingDirectory__);
                                if (fullPath == "" || !__fileSystem__.isFile (fullPath))                                    return "Invalid file name";
                                if (!__fileSystem__.userHasRightToAccessFile (fullPath, __homeDirectory__))                 return "Access denyed";
                                if (__fileSystem__.remove (fullPath))                                                       return fullPath + " deleted";
                                else                                                                                        return Cstring<300> ("Can't delete ") + fullPath;
                            }
                        #else
                            #error Telnet rm command is included but threadSafeFS.h is not! #include <threadSafeFS.h> prior to #including <telnetServer.h> and pass file system reference to telnet server constructor
                        #endif                           
                    #endif

                          
                        /*

                            const char *__passwd__ (char *userName) {
                                char password1 [USER_PASSWORD_MAX_LENGTH + 1];
                                char password2 [USER_PASSWORD_MAX_LENGTH + 1];
                                          
                                if (!strcmp (__userName__, userName)) { // user changing password for himself
                                    // read current password
                                    __echo__ = false;
                                    if (sendString ("Enter current password: ") <= 0)                                                   return ""; 
                                    if (recvLine (password1, sizeof (password1), true) != 13) { __echo__ = true;                  return "\r\nPassword not changed"; }
                                    __echo__ = true;
                                    // check if password is valid for user
                                    if (!userManagement.checkUserNameAndPassword (userName, password1))                           return "Wrong password"; 
                                } else {                         // root is changing password for another user
                                    // check if user exists with getUserHomeDirectory
                                    char homeDirectory [FILE_PATH_MAX_LENGTH + 1];
                                    if (!userManagement.getHomeDirectory (homeDirectory, sizeof (homeDirectory), userName))       return "User name does not exist"; 
                                }
                                // read new password twice
                                __echo__ = false;
                                if (sendString ("\r\nEnter new password: ") <= 0)                                                 return ""; 
                                if (recvLine (password1, sizeof (password1), true) != 13)   { __echo__ = true;                    return "\r\nPassword not changed"; }
                                if (sendString ("\r\nRe-enter new password: ") <= 0)                                              return ""; 
                                if (recvLine (password2, sizeof (password2), true) != 13)   { __echo__ = true;                    return "\r\nPassword not changed"; }
                                __echo__ = true;
                                // check passwords
                                if (strcmp (password1, password2))                                                                return "\r\nPasswords do not match";
                                    // change password
                                    if (userManagement.passwd (userName, password1))                                              return "\r\nPassword changed";
                                    else                                                                                          return "\r\nError changing password";  
                            }
                        */
                        

                    

            };


        private:

            #ifdef __THREAD_SAFE_FS__
                threadSafeFS::FS __fileSystem__;
            #endif
            Cstring<255> (*__getUserHomeDirectory__) (const Cstring<64>& userName, const Cstring<64>& password) = NULL;
            String (*__telnetCommandHandlerCallback__) (int argc, char *argv [], telnetConnection_t *tcn); // will be initialized in constructor

        public:

            telnetServer_t (
                            #ifdef __THREAD_SAFE_FS__
                                threadSafeFS::FS fileSystem,                                                                    // file system that FTP server would use
                            #endif
                            Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL,
                            String (*telnetCommandHandlerCallback) (int argc, char *argv [], telnetConnection_t *tcn) = NULL, // telnetCommadHandlerCallback function provided by calling program
                            int serverPort = 23,                                                                              // Telnet server port
                            bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,                                 // a reference to callback function that will be celled when new connection arrives 
                            bool runListenerInItsOwnTask = true                                                               // a calling program may repeatedly call accept itself to save some memory tat listener task would use
                           ) : tcpServer_t (serverPort, firewallCallback, runListenerInItsOwnTask),
                               #ifdef __THREAD_SAFE_FS__
                                   __fileSystem__ (fileSystem),
                               #endif
                               __getUserHomeDirectory__ (getUserHomeDirectory),
                               __telnetCommandHandlerCallback__ (telnetCommandHandlerCallback) {
            }

            tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) override {
                #define telnetServiceUnavailableReply (const char *) "Telnet service is currently unavailable.\r\nFree heap: %u bytes\r\nFree heap in one piece: %u bytes\r\n"

                telnetConnection_t *connection = new (std::nothrow) telnetConnection_t (
                                                                                        #ifdef __THREAD_SAFE_FS__
                                                                                            __fileSystem__, 
                                                                                        #endif
                                                                                        __getUserHomeDirectory__, 
                                                                                        connectionSocket, 
                                                                                        clientIP, 
                                                                                        serverIP, 
                                                                                        __telnetCommandHandlerCallback__);

                if (!connection) {
                    dmesgQueue << "[telnetServer] " << "can't create connection instance, out of memory";
                    char s [128];
                    sprintf (s, telnetServiceUnavailableReply, esp_get_free_heap_size (), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT));
                    send (connectionSocket, s, strlen (s), 0);
                    close (connectionSocket); // normally tcpConnection would do this but if it is not created we have to do it here since the connection was not created
                    return NULL;
                } 

                connection->setIdleTimeout (TELNET_CONNECTION_TIME_OUT);

                #define tskNORMAL_PRIORITY (tskIDLE_PRIORITY + 1)
                if (pdPASS != xTaskCreate ([] (void *thisInstance) {
                                                                        telnetConnection_t* ths = (telnetConnection_t *) thisInstance; // get "this" pointer
                                                                        xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                                                            __runningTcpConnections__ ++;
                                                                        xSemaphoreGive (getLwIpMutex ());

                                                                        ths->__runConnectionTask__ ();

                                                                        xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                                                                            __runningTcpConnections__ --;
                                                                        xSemaphoreGive (getLwIpMutex ());

                                                                        delete ths;
                                                                        vTaskDelete (NULL); // it is connection's responsibility to close itself
                                                                    }
                                            , "telnetConn", TELNET_CONNECTION_STACK_SIZE, connection, tskNORMAL_PRIORITY, NULL)) {
                    dmesgQueue << "[telnetServer] " << "can't create connection task, out of memory";

                    char s [128];
                    sprintf (s, telnetServiceUnavailableReply, esp_get_free_heap_size (), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT));
                    connection->sendString (s);
                    delete (connection); // normally tcpConnection would do this but if it is not running we have to do it here
                    return NULL;
                }

                return NULL; // success, but don't return connection, since it may already been closed and destructed by now
            }

            // accept any connection, the client will get notified in __createConnectionInstance__ if there arn't enough resources
            inline tcpConnection_t *accept () __attribute__((always_inline)) { return tcpServer_t::accept (); }

    };


    // define static member outside of class definition
    UBaseType_t telnetServer_t::telnetConnection_t::__lastHighWaterMark__ = TELNET_CONNECTION_STACK_SIZE;

#endif
