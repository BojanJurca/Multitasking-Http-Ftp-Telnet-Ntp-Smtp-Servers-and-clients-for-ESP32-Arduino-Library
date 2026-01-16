/*

  ntpClient.cpp

  This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library

  This library is based on Let's make a NTP Client in C: https://lettier.github.io/posts/2016-04-26-lets-make-a-ntp-client-in-c.html
  which I'm keeping here as close to the original as possible due to its comprehensive explanation.

  January 1, 2026, Bojan Jurca

*/


#include "ntpClient.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#if __has_include(<dmesg.hpp>) 
    #include <dmesg.hpp>
#endif


// missing function in LwIP
static const char *gai_strerror (int err) {
    switch (err) {
        case EAI_AGAIN:     return "temporary failure in name resolution";
        case EAI_BADFLAGS:  return "invalid value for ai_flags field";
        case EAI_FAIL:      return "non-recoverable failure in name resolution";
        case EAI_FAMILY:    return "ai_family not supported";
        case EAI_MEMORY:    return "memory allocation failure";
        case EAI_NONAME:    return "name or service not known";
        case EAI_SERVICE:   return "service not supported for ai_socktype";
        case EAI_SOCKTYPE:  return "ai_socktype not supported";
        default:            return "invalid gai_errno code";
    }
}


ntpClient_t::ntpClient_t (const char *ntpServer0,
                          const char *ntpServer1,
                          const char *ntpServer2) {
    strncpy (__ntpServer__ [0], ntpServer0, sizeof (__ntpServer__  [0]));
    if (ntpServer1)
        strncpy (__ntpServer__  [1], ntpServer1, sizeof (__ntpServer__  [1]));
    if (ntpServer2)
        strncpy (__ntpServer__  [2], ntpServer2, sizeof (__ntpServer__  [2]));
}

// synchronizes time with NTP server, returns error message or "" if OK
const char *ntpClient_t::syncTime () {
    const char *s;
    if (!* (s = syncTime (__ntpServer__ [0])))
        return "";

    delay (25);
    if (*__ntpServer__ [1] && !* (s = syncTime (__ntpServer__ [1])))
        return "";

    delay (25);
    if (*__ntpServer__ [2] && !* (s = syncTime (__ntpServer__ [2])))
        return "";

    return "NTP servers are not available";
}

// synchronizes time with NTP server, returns error message or "" if OK
const char *ntpClient_t::syncTime (int ntpServerIndex) {
    if (ntpServerIndex < 0 || ntpServerIndex > 2 || *__ntpServer__ [ntpServerIndex])
        return "invalid NTP server";
    return syncTime (__ntpServer__ [ntpServerIndex]);
}

// synchronizes time with NTP server, returns error message or "" if OK
const char *ntpClient_t::syncTime (const char *ntpServerName) {
    if (!WiFi.isConnected () || WiFi.localIP () == IPAddress (0, 0, 0, 0))
        return "not connected";

    // »Network Time Protocol«

    typedef struct {
        uint8_t li_vn_mode;
        uint8_t stratum;
        uint8_t poll;
        uint8_t precision;
        uint32_t rootDelay;
        uint32_t rootDispersion;
        uint32_t refId;
        uint32_t refTm_s;
        uint32_t refTm_f;
        uint32_t origTm_s;
        uint32_t origTm_f;
        uint32_t rxTm_s;
        uint32_t rxTm_f;
        uint32_t txTm_s;
        uint32_t txTm_f;
    } ntp_packet;

    // »The NTP message consists of a 384 bit or 48 byte data structure containing 17 fields. Note that the order of li, vn, and mode
    //  is important. We could use three bit fields but instead we’ll combine them into a single byte to avoid any implementation-defined
    //  issues involving endianness, LSB, and/or MSB.«

    // »Populate our Message«

    // Create and zero out the packet. All 48 bytes worth.
    ntp_packet packet = {};
    *((char *) &packet + 0) = 0x1b;

    // »First we zero-out or clear out the memory of our structure and then fill it in with leap indicator zero, version number three, and mode 3.
    // The rest we can leave blank and still get back the time from the server.«

    // »Setup our Socket and Server Data Structure«

    // Create a UDP socket, convert the host-name to an IP address, set the port number,
    // connect to the server, send the packet, and then read in the return packet.

    struct addrinfo hints, *res, *p;
    char ipstr [INET6_ADDRSTRLEN];
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
    int status = getaddrinfo (ntpServerName, NULL, &hints, &res);
    xSemaphoreGive (getLwIpMutex ());

    if (status != 0)
        return gai_strerror (status);

    // IP addresses for serverName
    bool isIPv6;
    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        if (p->ai_family == AF_INET) {
            isIPv6 = false;
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
            addr = & (ipv4->sin_addr);
        } else {
            isIPv6 = true;
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) p->ai_addr;
            addr = & (ipv6->sin6_addr);
        }
        inet_ntop (p->ai_family, addr, ipstr, sizeof (ipstr));
        break;
    }

    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
    freeaddrinfo (res);

    int sockfd;
    if (isIPv6)
        sockfd = socket (AF_INET6, SOCK_DGRAM, IPPROTO_IPV6);
    else
        sockfd = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sockfd < 0) {
        #if __has_include (<dmesg.hpp>)
            dmesgQueue << "[NTP] socket error: " << errno << " " << strerror (errno);
        #endif
        xSemaphoreGive (getLwIpMutex ());
        return "socket error";
    }

    // Setup socket timeout to 1s, but this won't limit the time we wait for NTP reply since th socket will be non-blocking
    struct timeval tout = {1, 0};
    setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof (tout));

    if (fcntl (sockfd, F_SETFL, O_NONBLOCK) == -1) {
        #if __has_include (<dmesg.hpp>)
            dmesgQueue << "[NTP] fcntl error: " << errno << " " << strerror (errno);
        #endif
        close (sockfd);
        xSemaphoreGive (getLwIpMutex ());
        return "fcntl error";
    }

    // connect to the server
    if (isIPv6) {
        struct sockaddr_in6 serv_addr = {};  // zero out the server address structure.
        serv_addr.sin6_family = AF_INET6;
        serv_addr.sin6_port = htons (123);  // convert the port number integer to network big-endian style and save it to the server address structure.
        if (inet_pton (AF_INET6, ipstr, &serv_addr.sin6_addr) <= 0) {
            #if __has_include(<dmesg.hpp>)
                dmesgQueue << "[NTP] invalid or not supported address " << ipstr;
            #endif
            close (sockfd);
            xSemaphoreGive (getLwIpMutex ());
            return "invalid or not supported address";
        }

        // »Before we can start communicating we have to setup our socket, server and server address structures. We will be using the
        //  User Datagram Protocol (versus TCP) for our socket since the server we are sending our message to is listening on port
        //  number 123 using UDP.«

        // Call up the server using its IP address and port number.
        if (connect (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
            #if __has_include(<dmesg.hpp>)
                dmesgQueue << "connect error: " << errno << " " << strerror (errno);
            #endif
            close (sockfd);
            xSemaphoreGive (getLwIpMutex ());
            return "connect error";
        }

        // »Send our Message to the Server«

        // Send the NTP packet
        int n;
        n = sendto (sockfd, (char *)&packet, sizeof(ntp_packet), 0, (struct sockaddr *)&serv_addr, sizeof (serv_addr));
        if (n < 0) {
            #if __has_include(<dmesg.hpp>)
                dmesgQueue << "sendto error: " << errno << " " << strerror (errno);
            #endif
            close (sockfd);
            xSemaphoreGive (getLwIpMutex ());
            return "sendto error";
        }

        xSemaphoreGive (getLwIpMutex ());

        // »With our message payload, socket, server and address setup, we can now send our message to the server.
        //  To do this, we write our 48 byte struct to the socket.«

        // »Read in the Return Message«

        // Wait and receive the packet back from the server. If n == -1, it failed.

        struct sockaddr_in6 from;
        socklen_t fromlen = sizeof (from);
        unsigned long startMillis = millis();
        while (true) {
            delay (25);
            if (millis() - startMillis > 1000) {
                xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                close (sockfd);
                xSemaphoreGive (getLwIpMutex ());
                return "time-out";
            }
            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            n = recvfrom (sockfd, (char *) &packet, sizeof (ntp_packet), 0, (struct sockaddr *) &from, (socklen_t *) &fromlen);
            xSemaphoreGive (getLwIpMutex ());
            if (n < 0) {
                if (errno == 11)  // EWOULDBLOCK || EAGAIN
                    continue;
                #if __has_include(<dmesg.hpp>)
                    dmesgQueue << "recvfrom error: " << errno << " " << strerror(errno);
                #endif
                xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                close (sockfd);
                xSemaphoreGive (getLwIpMutex ());
                return "recvfrom error";
            }
            // Did we get the reply from the expected server?
            if (memcmp (&from.sin6_addr, &serv_addr.sin6_addr, sizeof (struct in6_addr)) == 0 && from.sin6_port == serv_addr.sin6_port)
                break;
        }

    } else { // IPv4

        struct sockaddr_in serv_addr = {};  // zero out the server address structure.
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons (123);  // convert the port number integer to network big-endian style and save it to the server address structure.
        if (inet_pton (AF_INET, ipstr, &serv_addr.sin_addr) <= 0) {
            #if __has_include(<dmesg.hpp>)
                dmesgQueue << "[NTP] invalid or not supported address " << ipstr;
            #endif
            close (sockfd);
            xSemaphoreGive (getLwIpMutex ());
            return "invalid or not supported address";
        }

        // »Before we can start communicating we have to setup our socket, server and server address structures. We will be using the
        //  User Datagram Protocol (versus TCP) for our socket since the server we are sending our message to is listening on port
        //  number 123 using UDP.«

        // Call up the server using its IP address and port number.
        if (connect (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
            #if __has_include(<dmesg.hpp>)
                dmesgQueue << "[NTP] connect error: " << errno << " " << strerror (errno);
            #endif
            close (sockfd);
            xSemaphoreGive (getLwIpMutex ());
            return "connect error";
        }

        // »Send our Message to the Server«

        // Send it the NTP packet it wants. If n == -1, it failed.
        int n;
        n = sendto (sockfd, (char *) &packet, sizeof (ntp_packet), 0, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
        if (n < 0) {
            #if __has_include(<dmesg.hpp>)
                dmesgQueue << "[NTP] sendto error: " << errno << " " << strerror (errno);
            #endif
            close (sockfd);
            xSemaphoreGive (getLwIpMutex ());
            return "sendto error";
        }

        xSemaphoreGive (getLwIpMutex ());

        // »With our message payload, socket, server and address setup, we can now send our message to the server.
        //  To do this, we write our 48 byte struct to the socket.«

        // »Read in the Return Message«

        // Wait and receive the packet back from the server. If n == -1, it failed.

        struct sockaddr_in from;
        socklen_t fromlen = sizeof (from);
        unsigned long startMillis = millis ();
        while (true) {
            delay(25);
            if (millis () - startMillis > 1000) {
                xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
                close (sockfd);
                xSemaphoreGive (getLwIpMutex ());
                return "time-out";
            }
            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            n = recvfrom (sockfd, (char *) &packet, sizeof (ntp_packet), 0, (struct sockaddr *) &from, (socklen_t *) &fromlen);
            xSemaphoreGive (getLwIpMutex ());
            if (n < 0) {
                if (errno == 11)  // EWOULDBLOCK || EAGAIN
                    continue;
                #if __has_include(<dmesg.hpp>)
                    dmesgQueue << "recvfrom error: " << errno << " " << strerror(errno);
                #endif
            xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
            close (sockfd);
            xSemaphoreGive (getLwIpMutex ());
            return "recvfrom error";
            }
            // Did we get the reply from the expected server?
            if (from.sin_addr.s_addr == serv_addr.sin_addr.s_addr && from.sin_port == serv_addr.sin_port)
            break;
        }
    }

    xSemaphoreTake (getLwIpMutex (), portMAX_DELAY);
    close (sockfd);
    xSemaphoreGive (getLwIpMutex ());


    // »Now that our message is sent, we block or wait for the response by reading from the socket. The message we get back should be the same
    //  size as the message we sent. We will store the incoming message in packet just like we stored our outgoing message.«

    // »Parse the Return Message«

    // These two fields contain the time-stamp seconds as the packet left the NTP server.
    // The number of seconds correspond to the seconds passed since 1900.
    // ntohl() converts the bit/byte order from the network's to host's "endianness".

    // After receiving packet:
    packet.txTm_s = ntohl (packet.txTm_s);
    packet.txTm_f = ntohl (packet.txTm_f);

    #define NTP_TIMESTAMP_DELTA 2208988800
    struct timeval txTm = { (time_t) (packet.txTm_s - NTP_TIMESTAMP_DELTA), 0 };
    settimeofday (&txTm, NULL);

    #if __has_include (<dmesg.hpp>)
        dmesgQueue << "[NTP] synchronized with " << ntpServerName << " (" << ipstr << ")";
    #endif
    return "";
}
