/*

  threadSafeFS.cpp

  This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


  A FS wrapper with mutex for multitasking.

  January 1, 2026, Bojan Jurca

*/


#include "threadSafeFS.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


// singleton mutex
SemaphoreHandle_t getFsMutex () {
    static SemaphoreHandle_t semaphore = xSemaphoreCreateMutex ();
    return semaphore;
}


// File implementation

threadSafeFS::File::File () : fs::File () {}
threadSafeFS::File::File (fs::File&& f) : fs::File (std::move (f)) {}

size_t threadSafeFS::File::write (const uint8_t* buf, size_t len) {
    xSemaphoreTake (getFsMutex(), portMAX_DELAY);
    size_t s = fs::File::write (buf, len);
    xSemaphoreGive (getFsMutex ());
    return s;
}

size_t threadSafeFS::File::write (uint8_t b) {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    size_t s = fs::File::write (b);
    xSemaphoreGive (getFsMutex ());
    return s;
}

size_t threadSafeFS::File::read (uint8_t* buf, size_t len) {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    size_t s = fs::File::read (buf, len);
    xSemaphoreGive (getFsMutex ());
    return s;
}

int threadSafeFS::File::read () {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    int i = fs::File::read ();
    xSemaphoreGive (getFsMutex ());
    return i;
}

int threadSafeFS::File::available () {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    int i = fs::File::available ();
    xSemaphoreGive (getFsMutex ());
    return i;
}

void threadSafeFS::File::flush () {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    fs::File::flush ();
    xSemaphoreGive (getFsMutex ());
}

bool threadSafeFS::File::seek (uint32_t pos, SeekMode mode) {
    xSemaphoreTake (getFsMutex(), portMAX_DELAY);
    bool b = fs::File::seek (pos, mode);
    xSemaphoreGive (getFsMutex ());
    return b;
}

size_t threadSafeFS::File::position () {
    xSemaphoreTake (getFsMutex(), portMAX_DELAY);
    size_t s = fs::File::position ();
    xSemaphoreGive (getFsMutex ());
    return s;
}

size_t threadSafeFS::File::size () {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    size_t s = fs::File::size ();
    xSemaphoreGive (getFsMutex ());
    return s;
}

void threadSafeFS::File::close () {
    xSemaphoreTake (getFsMutex(), portMAX_DELAY);
    fs::File::close ();
    xSemaphoreGive (getFsMutex ());
}

bool threadSafeFS::File::isDirectory () {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    bool b = fs::File::isDirectory ();
    xSemaphoreGive (getFsMutex ());
    return b;
}

threadSafeFS::File threadSafeFS::File::openNextFile (const char* mode) {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    fs::File f = fs::File::openNextFile (mode);
    xSemaphoreGive (getFsMutex ());
    return File (std::move (f));
}


// additional write/print helpers

size_t threadSafeFS::File::write (const char* buf) {
    return write ((const uint8_t*) buf, strlen (buf));
}

size_t threadSafeFS::File::write (String& s) {
    return write (s.c_str ());
}

size_t threadSafeFS::File::print (const char* buf) {
    return write ((const uint8_t*)buf, strlen (buf));
}

size_t threadSafeFS::File::print (String& s) {
    return write (s.c_str ());
}

size_t threadSafeFS::File::print (const int16_t& value) {
    char buf [7];
    sprintf (buf, "%i", value);
    return print (buf);
}

size_t threadSafeFS::File::print (const uint16_t& value) {
    char buf [6];
    sprintf (buf, "%u", value);
    return print (buf);
}

size_t threadSafeFS::File::print (const int32_t& value) {
    char buf [12];
    sprintf (buf, "%li", value);
    return print (buf);
}

size_t threadSafeFS::File::print (const uint32_t& value) {
    char buf [11];
    sprintf (buf, "%lu", value);
    return print (buf);
}

size_t threadSafeFS::File::print (const int64_t& value) {
    char buf [21];
    sprintf (buf, "%lli", value);
    return print (buf);
}

size_t threadSafeFS::File::print (const uint64_t& value) {
    char buf [21];
    sprintf (buf, "%llu", value);
    return print (buf);
}

size_t threadSafeFS::File::print (const float& value) {
    char buf [61];
    sprintf (buf, "%f", value);
    return print (buf);
}

size_t threadSafeFS::File::print (const double& value) {
    char buf [331];
    sprintf (buf, "%lf", value);
    return print (buf);
}

size_t threadSafeFS::File::print (const long double& value) {
    char buf [331];
    sprintf (buf, "%Lf", value);
    return print (buf);
}


// FS implementation

threadSafeFS::FS::FS (fs::FS& fileSystem) : __fileSystem__ (fileSystem) {}

threadSafeFS::File threadSafeFS::FS::open (const char* path, const char* mode) {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    File f = __fileSystem__.open (path, mode);
    xSemaphoreGive (getFsMutex ());
    return f;
}

threadSafeFS::File threadSafeFS::FS::open (const String& path, const char* mode) {
    return open (path.c_str (), mode);
}

bool threadSafeFS::FS::exists (const char* path) {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    bool b = __fileSystem__.exists (path);
    xSemaphoreGive (getFsMutex ());
    return b;
}

bool threadSafeFS::FS::exists (const String& path) {
    return exists (path.c_str ());
}

bool threadSafeFS::FS::remove (const char* path) {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    bool b = __fileSystem__.remove (path);
    xSemaphoreGive (getFsMutex ());
    return b;
}

bool threadSafeFS::FS::remove (const String& path) {
    return remove (path.c_str ());
}

bool threadSafeFS::FS::rename (const char* from, const char* to) {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    bool b = __fileSystem__.rename (from, to);
    xSemaphoreGive (getFsMutex ());
    return b;
}

bool threadSafeFS::FS::rename (const String& from, const String& to) {
    return rename (from.c_str (), to.c_str ());
}

bool threadSafeFS::FS::mkdir (const char* path) {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    bool b = __fileSystem__.mkdir (path);
    xSemaphoreGive (getFsMutex ());
    return b;
}

bool threadSafeFS::FS::mkdir (const String& path) {
    return mkdir (path.c_str ());
}

bool threadSafeFS::FS::rmdir (const char* path) {
    xSemaphoreTake (getFsMutex (), portMAX_DELAY);
    bool b = __fileSystem__.rmdir (path);
    xSemaphoreGive (getFsMutex ());
    return b;
}

bool threadSafeFS::FS::rmdir (const String& path) {
    return rmdir (path.c_str ());
}

bool threadSafeFS::FS::mounted () {
    return open ("/", FILE_READ);
}

Cstring<255> threadSafeFS::FS::makeFullPath (const char *relativePath, const char *workingDirectory) {
    Cstring<300> relPath = relativePath;
    // remove the first and the last " if they exist
    size_t length = relPath.length ();
    if (relPath [0] == '\"' && relPath [length - 1] == '\"' && length > 1)
      relPath = relPath.substr (1, length - 2);
    
    Cstring<300> fullPath;
    // if relateivePath starts with "/" and doesn't contain "." it already is a ful path
    if (relativePath [0] == '/' && !strstr (relativePath, ".")) {
        fullPath = relativePath;
    } else {
        fullPath = workingDirectory;
        if (fullPath [fullPath.length () - 1] != '/')
            fullPath += '/';
        if (relativePath [0] != '/')
            fullPath += relPath;
        else
            fullPath += &relPath [1];
    }
    if (fullPath [fullPath.length () - 1] != '/')
        fullPath += '/';        
    // check if fullPath is valid (didn't overflow)
    if (fullPath.errorFlags ())
        return "";
    // remove all "./" substrings
    int i = 0;
    while (i >= 0) {
        switch ((i = fullPath.indexOf ("/./"))) {
            case -1:    
                        break;
            default: 
                        strcpy (&fullPath [i + 1], &fullPath [i + 3]);
                        break;
        }
    }
    // resolve all "../" substrings
    i = 0;
    while (i >= 0) {
        switch ((i = fullPath.indexOf ("/../"))) {
            case -1:    
                        break;
            case 0: 
                        // invalid relative path
                        return "";
            default: 
                        // find the last "/" before i
                        int j = 0;
                        for (int k = j; k < i; k++)
                            if (fullPath [k] == '/')
                                j = k;
                        strcpy (&fullPath [j], &fullPath [i + 3]);
                        break;
        }
    }

    // remove the last "/"
    if (fullPath != "/")
        fullPath [fullPath.length () - 1] = 0;

    return fullPath;
}

bool threadSafeFS::FS::isFile (Cstring<255>& fullPath) {
    threadSafeFS::File f = open (fullPath, FILE_READ);
    if (!f) return false;
    return !f.isDirectory ();
}

bool threadSafeFS::FS::isDirectory (Cstring<255>& fullPath) {
    threadSafeFS::File f = open (fullPath, FILE_READ);
    if (!f) return false;
    return f.isDirectory ();
}

bool threadSafeFS::FS::userHasRightToAccessFile (const char *fullPath, const char *homeDirectory) {
    return strstr (fullPath, homeDirectory) == fullPath;
}

bool threadSafeFS::FS::userHasRightToAccessDirectory (Cstring<255> fullPath, Cstring<255> homeDirectory) {
    if (fullPath [fullPath.length () - 1] != '/') fullPath += '/';
    if (homeDirectory [homeDirectory.length () - 1] != '/') homeDirectory += '/';
    return userHasRightToAccessFile (fullPath, homeDirectory);
}

// returns UNIX like text with file information - this is what FTP clients expect
Cstring<300> threadSafeFS::FS::fileInformation (const char *fileOrDirectory, bool showFullPath) {
    Cstring<300> s;
    threadSafeFS::File f = open (fileOrDirectory, FILE_READ);
    if (f) {
        struct tm fTime = {};
        time_t lTime = f.getLastWrite ();
        localtime_r (&lTime, &fTime);
        sprintf (s, "%crw-rw-rw-   1 root     root          %7lu ", f.isDirectory () ? 'd' : '-', f.size ());
        strftime ((char *) s.c_str () + strlen (s.c_str ()), 25, " %b %d %H:%M      ", &fTime);
        if (showFullPath || !strcmp (fileOrDirectory, "/")) {
            s += fileOrDirectory;
        } else {
            int lastSlash = 0;
            for (int i = 1; fileOrDirectory [i]; i++)
                if (fileOrDirectory [i] == '/') 
                lastSlash = i;
            s += fileOrDirectory + lastSlash + 1;
        }
        f.close ();
    }
    return s;
}


// fprintf implementation

size_t fprintf (threadSafeFS::File &f, const char *fmt, ...) {
    if (!f || !fmt)
        return 0; // -1

    char buf [500];
    va_list args;
    va_start (args, fmt);
    int len = vsnprintf (buf, sizeof (buf), fmt, args);
    va_end (args);

    if (len < 0)
        return 0; // -1

    return f.print (buf);
}
