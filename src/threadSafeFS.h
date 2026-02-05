/*

  threadSafeFS.h

  This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


  A FS wrapper with mutex for multitasking.

  February 6, 2026, Bojan Jurca

*/


#pragma once
#ifndef __THREAD_SAFE_FS__
  #define __THREAD_SAFE_FS__

  #include <FS.h>
  #include <Cstring.hpp>
  #include <list.hpp>

  // global mutex for thread-safe FS
  SemaphoreHandle_t getFsMutex ();

  namespace threadSafeFS {

      class FS;   // forward declaration

      class File : public fs::File {
      private:
          FS* __threadSafeFileSystem__ = nullptr;   // pointer, allows default constructor

      public:
          File () = default;                        // invalid file, allowed
          File (FS& fs, fs::File&& f);              // valid file created by FS::open ()

          ~File ();

          operator bool () const { return __threadSafeFileSystem__ != NULL && fs::File::operator bool (); } 

          // basic overrides
          size_t write (const uint8_t* buf, size_t len);
          size_t write (uint8_t b);
          size_t read (uint8_t* buf, size_t len);
          int read ();
          int available ();
          void flush ();
          bool seek (uint32_t pos, SeekMode mode);
          size_t position ();
          size_t size ();
          void close ();
          bool isDirectory ();
          File openNextFile (const char* mode = FILE_READ);

          // additional helpers
          size_t write (const char* buf);
          size_t write (String& s);

          size_t print (const char* buf);
          size_t print (String& s);
          size_t print (const int16_t& value);
          size_t print (const uint16_t& value);
          size_t print (const int32_t& value);
          size_t print (const uint32_t& value);
          size_t print (const int64_t& value);
          size_t print (const uint64_t& value);
          size_t print (const float& value);
          size_t print (const double& value);
          size_t print (const long double& value);

          template<typename T>
          size_t println (T value) {
              return print (value) + print ("\r\n");
          }

      };


      class FS {
          friend class File;

      private:
          fs::FS& __fileSystem__;

      public:
          // lists of opened files
          list<const char*> readOpenedFiles;
          list<const char*> writeOpenedFiles;

          FS (fs::FS& fileSystem);

          File open (const char* path, const char* mode = FILE_READ);
          File open (const String& path, const char* mode = FILE_READ);

          bool exists (const char* path);
          bool exists (const String& path);

          bool remove (const char* path);
          bool remove (const String& path);

          bool rename (const char* from, const char* to);
          bool rename (const String& from, const String& to);

          bool mkdir (const char* path);
          bool mkdir (const String& path);

          bool rmdir (const char* path);
          bool rmdir (const String& path);

          bool mounted ();

          Cstring<255> makeFullPath (const char* relativePath, const char* workingDirectory);

          bool isFile (Cstring<255>& fullPath);
          bool isDirectory (Cstring<255>& fullPath);

          bool userHasRightToAccessFile (const char* fullPath, const char* homeDirectory);
          bool userHasRightToAccessDirectory (Cstring<255> fullPath, Cstring<255> homeDirectory);

          Cstring<300> fileInformation (const char* fileOrDirectory, bool showFullPath = false);

          bool readConfiguration (char* buffer, size_t bufferSize, const char* fileName);
      };

      // fprintf
      size_t fprintf (threadSafeFS::File& f, const char* fmt, ...);

  }

#endif
