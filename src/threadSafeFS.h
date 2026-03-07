/*

  threadSafeFS.h

  This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


  A FS wrapper with mutex for multitasking.

  March 12, 2026, Bojan Jurca

*/


#pragma once
#ifndef __THREAD_SAFE_FS__
    #define __THREAD_SAFE_FS__


    #include <FS.h>
    #include <ostream.hpp>
    #include <Cstring.hpp>
    #include <list.hpp>


    SemaphoreHandle_t getFsMutex ();

    namespace threadSafeFS {

        class FS;   // forward declaration

        class File {
            private:
                fs::File* __file__ = NULL;
                FS* __threadSafeFileSystem__ = NULL;

            public:
                File () = default;
                File (FS& fs, fs::File&& f);
                File (const File&) = delete;
                File (File&& other) noexcept;
                File& operator = (File&& other) noexcept;
                File& operator = (const File&) = delete;

                ~File ();

                operator bool () const { return __file__ && __threadSafeFileSystem__; }

                // basic wrappers
                Cstring<255> path ();
                Cstring<255> name ();
                time_t getLastWrite ();
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
                // File openNextFile (const char* mode = FILE_READ);

                // helpers
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

                size_t println () {
                    return println ();
                }

                // iterate through a directory
                class Iterator {
                    public:
                        Iterator ();
                        Iterator (FS* fs, fs::File* dir);
                        bool operator != (const Iterator& other) const;
                        File operator *();
                        Iterator& operator ++();
                    private:
                        fs::File* __dir__;
                        fs::File __current__;
                        FS* __fs__;
                        bool __end__ = false;

                        // for SPIFFS only
                        list<Cstring<31>> subDirectories;
                };

                Iterator begin ();
                Iterator end ();
        };


        class FS {
            friend class File;

        private:
            fs::FS& __fileSystem__;

        public:
            list<Cstring<255>> readOpenedFiles;
            list<Cstring<255>> writeOpenedFiles;

            FS (fs::FS& fileSystem); // for file systems

            /*
            bool format ();
            bool begin (bool formatOnFail = false);
            void end ();
            */


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

            bool isFile (const char *fullPath);
            bool isFile (Cstring<255>& fullPath);
            bool isDirectory (const char *fullPath);
            bool isDirectory (Cstring<255>& fullPath);

            bool userHasRightToAccessFile (const char* fullPath, const char* homeDirectory);
            bool userHasRightToAccessDirectory (Cstring<255> fullPath, Cstring<255> homeDirectory);

            Cstring<300> fileInformation (const char* fileOrDirectory, bool showFullPath = false);

            bool readConfiguration (char* buffer, size_t bufferSize, const char* fileName);
        };


    }; // namespace

    // fprintf compatibility with standard C
    size_t fprintf (threadSafeFS::File &f, const char *fmt, ...);

#endif