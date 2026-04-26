/*

    dmesg.hpp

    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library


      - Use dmesg telnet command to display messages in the dmesg message queue.

    March 12, 2026, Bojan Jurca
    
*/


#pragma once 

#ifndef __DMESG__
    #define __DMESG__


    #include <WiFi.h>
    #include <rom/rtc.h>
    #include <ostream.hpp>
    #include <Cstring.hpp>      // include LightweightSTL library: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
    #include "threadSafeCircularQueue.hpp"


    // ----- TUNNING PARAMETERS -----

    #ifndef DMESG_MAX_MESSAGE_LENGTH
        #define DMESG_MAX_MESSAGE_LENGTH 88     // max length of each message
    #endif
    #ifndef DMESG_CIRCULAR_QUEUE_LENGTH
        // #if CONFIG_IDF_TARGET_ESP32S2
            #define DMESG_CIRCULAR_QUEUE_LENGTH 21 // how may massages we keep on circular queue, ESP32 S2 has limited memory
        // #else
        //     #define DMESG_CIRCULAR_QUEUE_LENGTH 42 // how may massages we keep on circular queue
        // #endif
    #endif


    // keep the following information for each entry 
    struct dmesgQueueEntry_t {
        unsigned long milliseconds;
        time_t time;
        Cstring<DMESG_MAX_MESSAGE_LENGTH> message; 

        template<typename T>
        inline dmesgQueueEntry_t& operator << (const T& value) {
            this->message += value;
            return *this;
        }

        inline dmesgQueueEntry_t& operator << (const char *value) {    
            this->message += value;
            return *this;
        }

        inline dmesgQueueEntry_t& operator << (const String& value) {
            this->message += (const char *) value.c_str ();
            return *this;
        }

        template<size_t N>
        inline dmesgQueueEntry_t& operator << (const Cstring<N>& value) {
            this->message += (char *) &value;
            return *this;
        }

        template<size_t N>
        inline dmesgQueueEntry_t& operator << (const char (&value) [N]) {
            this->message += static_cast<const char *> (value);
            return *this;
        }

        // integers

        inline dmesgQueueEntry_t& operator << (const int16_t& value) {
            this->message += value;
            return *this;
        }

        inline dmesgQueueEntry_t& operator << (const uint16_t& value) {
            this->message += value;
            return *this;
        }

        inline dmesgQueueEntry_t& operator << (const int32_t& value) {
            this->message += value;
            return *this;
        }

        inline dmesgQueueEntry_t& operator << (const uint32_t& value) {
            this->message += value;
            return *this;
        }

        inline dmesgQueueEntry_t& operator << (const int64_t& value) {
            this->message += value;
            return *this;
        }

        inline dmesgQueueEntry_t& operator << (const uint64_t& value) {
            this->message += value;
            return *this;
        }

        // floats

        inline dmesgQueueEntry_t& operator << (const float& value) {
            this->message += value;
            return *this;
        }

        inline dmesgQueueEntry_t& operator << (const double& value) {
            this->message += value;
            return *this;
        }

        inline dmesgQueueEntry_t& operator << (const long double& value) {
            this->message += value;
            return *this;
        }

        // time

        inline dmesgQueueEntry_t& operator << (const struct tm& value) {
            this->message += value;
            return *this;
        }

        // IPAddress

        inline dmesgQueueEntry_t& operator << (const IPAddress& ip) {
            this->message += ip [0];
            this->message += '.';
            this->message += ip [1];
            this->message += '.';
            this->message += ip [2];
            this->message += '.';
            this->message += ip [3];
            return *this;
        }

    };


    // declare dmesg circular queue 
    template<size_t maxSize> class dmesgQueue_t : public threadSafeCircularQueue<dmesgQueueEntry_t, maxSize> {

        public:

            // constructor - insert the first entries
            dmesgQueue_t () : threadSafeCircularQueue<dmesgQueueEntry_t, maxSize> () { 
                #if CONFIG_IDF_TARGET_ESP32
                        #define ESP32TYPE "ESP32"
                    #elif CONFIG_IDF_TARGET_ESP32S2
                        #define ESP32TYPE "ESP32-S2"
                    #elif CONFIG_IDF_TARGET_ESP32S3
                        #define ESP32TYPE "ESP32-S3"
                    #elif CONFIG_IDF_TARGET_ESP32C2
                        #define ESP32TYPE "ESP32-C2"
                    #elif CONFIG_IDF_TARGET_ESP32C3
                        #define ESP32TYPE "ESP32-C3"
                    #elif CONFIG_IDF_TARGET_ESP32C6
                        #define ESP32TYPE "ESP32-C6"
                    #elif CONFIG_IDF_TARGET_ESP32H2
                        #define ESP32TYPE "ESP32-H2"
                    #else
                        #define ESP32TYPE "ESP32-other"
                    #endif

                /* CPU frequency is not reported correctly at startup
                #ifdef HOSTNAME
                    this->operator << ("[" ESP32TYPE "] " HOSTNAME " runing at: ") << (int) ESP.getCpuFreqMHz () << + " MHz";
                #else
                    this->operator << ("[" ESP32TYPE "] runing at: ") << (int) ESP.getCpuFreqMHz () << + " MHz";
                #endif
                */

                (*this) << "[" ESP32TYPE "] reset reason: " << __resetReason__ ( esp_reset_reason () );
        
                (*this) << "[" ESP32TYPE "] wakeup reason: " << __wakeupReason__ ();

                (*this) << "[" ESP32TYPE "] free heap at startup: " << esp_get_free_heap_size () << " bytes";
                if (heap_caps_get_free_size (MALLOC_CAP_SPIRAM) == 0 && psramInit ())
                    (*this) << "[" ESP32TYPE "] free PSRAM at startup: " << heap_caps_get_free_size (MALLOC_CAP_SPIRAM) << " bytes";
                else
                    (*this) << "[" ESP32TYPE "] PSRAM not installed";

                time_t t = time (NULL);
                if (t > 1600000000) { // 1600000000 ~2020
                    struct tm st;
                    gmtime_r (&t, &st);
                    (*this) << "[time] internal RTC at startup: " << st << " UTC";
                } else {
                    (*this) << "[time] internal RTC time unknown at startup";
                }
            }

            inline dmesgQueueEntry_t& operator << (const char *value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const String& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), (const char *) value.c_str () } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            template<size_t N>
            inline dmesgQueueEntry_t& operator << (const Cstring<N>& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), (char *) &value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            template<size_t N>
            inline dmesgQueueEntry_t& operator << (const char (&value) [N]) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), static_cast<const char *> (value) } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            // integers

            inline dmesgQueueEntry_t& operator << (const int16_t& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const uint16_t& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const int32_t& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const uint32_t& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const int64_t& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const uint64_t& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            // floats

            inline dmesgQueueEntry_t& operator << (const float& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const double& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const long double& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            // time

            inline dmesgQueueEntry_t& operator << (const struct tm& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            // IPAddress

            inline dmesgQueueEntry_t& operator << (const IPAddress& ip) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                char buf [20]; // [INET_ADDRSTRLEN];
                sprintf (buf, "%i.%i.%i.%i", ip [0], ip [1], ip [2], ip [3]);
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), buf } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

        private:

            // returns reset reason (this may help with debugging)
            const char *__resetReason__ (esp_reset_reason_t reason) {
                switch (reason) {
                    case ESP_RST_UNKNOWN:   return "UNKNOWN - 0, Reset reason can not be determined";
                    case ESP_RST_POWERON:   return "POWERON_RESET - 1, Vbat power on reset";
                    case ESP_RST_EXT:       return "EXT_RESET - 2, Reset by external pin";
                    case ESP_RST_SW:        return "SW_RESET - 3, Software reset via esp_restart";
                    case ESP_RST_PANIC:     return "PANIC_RESET - 4, Software reset due to exception/panic";
                    case ESP_RST_INT_WDT:   return "INT_WDT_RESET - 5, Interrupt watchdog reset";
                    case ESP_RST_TASK_WDT:  return "TASK_WDT_RESET - 6, Task watchdog reset";
                    case ESP_RST_WDT:       return "WDT_RESET - 7, Other watchdog reset";
                    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP_RESET - 8, Wakeup from deep sleep";
                    case ESP_RST_BROWNOUT:  return "BROWNOUT_RESET - 9, Brownout reset (software or hardware)";
                    case ESP_RST_SDIO:      return "SDIO_RESET - 10, Reset over SDIO";
                    default:                return "RESET REASON UNKNOWN";
                }
            }

            // returns wakeup reason (this may help with debugging)
            const char *__wakeupReason__ () {
                esp_sleep_wakeup_cause_t wakeup_reason;
                wakeup_reason = esp_sleep_get_wakeup_cause ();
                switch (wakeup_reason) {
                    case ESP_SLEEP_WAKEUP_EXT0:     return "ESP_SLEEP_WAKEUP_EXT0 - wakeup caused by external signal using RTC_IO";
                    case ESP_SLEEP_WAKEUP_EXT1:     return "ESP_SLEEP_WAKEUP_EXT1 - wakeup caused by external signal using RTC_CNTL";
                    case ESP_SLEEP_WAKEUP_TIMER:    return "ESP_SLEEP_WAKEUP_TIMER - wakeup caused by timer";
                    case ESP_SLEEP_WAKEUP_TOUCHPAD: return "ESP_SLEEP_WAKEUP_TOUCHPAD - wakeup caused by touchpad";
                    case ESP_SLEEP_WAKEUP_ULP:      return "ESP_SLEEP_WAKEUP_ULP - wakeup caused by ULP program";
                    default:                        return "WAKEUP REASON UNKNOWN - wakeup was not caused by deep sleep";
                }
            }

    };

    #ifdef __OSTREAM_HPP__

        inline ostream& operator << (ostream& os, const dmesgQueueEntry_t& entry) {
            return os << entry.message << endl;
        }

        template<size_t maxSize>
        inline ostream& operator << (ostream& os, const dmesgQueue_t<maxSize>& queue) {
            // if (queue.empty ()) return os; // don't need to check, dmesgQueue is never empty
            return os << queue.back () << endl;
        }

    #endif


    // create a working singleton instance
    inline dmesgQueue_t<DMESG_CIRCULAR_QUEUE_LENGTH>& __getDmesgQueueInstance__ () {
        static dmesgQueue_t<DMESG_CIRCULAR_QUEUE_LENGTH> instance;
        return instance;
    }

    #if __cplusplus >= 201703L
        inline dmesgQueue_t<DMESG_CIRCULAR_QUEUE_LENGTH>& dmesgQueue = __getDmesgQueueInstance__ ();
    #else
        static dmesgQueue_t<DMESG_CIRCULAR_QUEUE_LENGTH>& dmesgQueue = __getDmesgQueueInstance__ ();
    #endif

#endif
