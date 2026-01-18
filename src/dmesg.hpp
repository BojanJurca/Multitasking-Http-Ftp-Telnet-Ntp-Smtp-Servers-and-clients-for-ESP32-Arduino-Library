/*
 
    dmesg.hpp

    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino

      - Use dmesgQueue.push_back (...) functions to put system messages in circular message queue.

      - Use dmesg telnet command to display messages in the dmesg message queue.

    January 1, 2026, Bojan Jurca
    
*/


#pragma once 

#ifndef __DMESG__
    #define __DMESG__

    
    #include <rom/rtc.h>
    #include <Cstring.hpp>      // include LightweightSTL library: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
    #include "threadSafeCircularQueue.hpp"


    // ----- TUNNING PARAMETERS -----

    #ifndef DMESG_MAX_MESSAGE_LENGTH
        #define DMESG_MAX_MESSAGE_LENGTH 88     // max length of each message
    #endif
    #ifndef DMESG_CIRCULAR_QUEUE_LENGTH
        #define DMESG_CIRCULAR_QUEUE_LENGTH 42 // how may massages we keep on circular queue
    #endif


    // keep the following information for each entry 
    struct dmesgQueueEntry_t {
        unsigned long milliseconds;
        time_t        time;
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

        /*
        inline bool operator == (const dmesgQueueEntry_t& other) {
            return this->milliseconds == other.milliseconds && this->message == other.message;
        }
        */

    };


    // define dmesg circular queue 
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

                #if CONFIG_FREERTOS_UNICORE // CONFIG_FREERTOS_UNICORE == 1 => 1 core ESP32
                    this->operator << ("[" ESP32TYPE "] CPU0 reset reason: ") << __resetReason__ (rtc_get_reset_reason (0));
                #else // CONFIG_FREERTOS_UNICORE == 0 => 2 core ESP32
                    this->operator << ("[" ESP32TYPE "] CPU0 reset reason: ") << __resetReason__ (rtc_get_reset_reason (0));
                    this->operator << ("[" ESP32TYPE "] CPU1 reset reason: ") << __resetReason__ (rtc_get_reset_reason (1));
                #endif            
        
                this->operator << ("[" ESP32TYPE "] wakeup reason: ") << __wakeupReason__ ();

                this->operator << ("[" ESP32TYPE "] free heap at startup: ") << esp_get_free_heap_size ();
                if (heap_caps_get_free_size (MALLOC_CAP_SPIRAM) == 0 && psramInit ())
                    this->operator << ("[" ESP32TYPE "] free PSRAM at startup: ") << heap_caps_get_free_size (MALLOC_CAP_SPIRAM);
                else
                    this->operator << ("[" ESP32TYPE "] PSRAM not installed");

                time_t t = time (NULL);
                if (t > 1748500189)
                    this->operator << ("[time] internal RTC: ") << t;
                else
                    this->operator << ("[time] internal RTC time unknown");
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

            inline dmesgQueueEntry_t& operator << (const int& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const unsigned int& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const long& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

            inline dmesgQueueEntry_t& operator << (const unsigned long& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }

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

            inline dmesgQueueEntry_t& operator << (const time_t& value) {
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Lock ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::push_back ( { millis (), time (NULL), value } );
                dmesgQueueEntry_t& r = threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::back ();
                threadSafeCircularQueue<dmesgQueueEntry_t, maxSize>::Unlock ();
                return r;
            }


        private:

            // returns reset reason (this may help with debugging)
            const char *__resetReason__ (RESET_REASON reason) {
                switch (reason) {
                    case 1:  return "POWERON_RESET - 1, Vbat power on reset";
                    case 3:  return "SW_RESET - 3, Software reset digital core";
                    case 4:  return "OWDT_RESET - 4, Legacy watch dog reset digital core";
                    case 5:  return "DEEPSLEEP_RESET - 5, Deep Sleep reset digital core";
                    case 6:  return "SDIO_RESET - 6, Reset by SLC module, reset digital core";
                    case 7:  return "TG0WDT_SYS_RESET - 7, Timer Group0 Watch dog reset digital core";
                    case 8:  return "TG1WDT_SYS_RESET - 8, Timer Group1 Watch dog reset digital core";
                    case 9:  return "RTCWDT_SYS_RESET - 9, RTC Watch dog Reset digital core";
                    case 10: return "INTRUSION_RESET - 10, Instrusion tested to reset CPU";
                    case 11: return "TGWDT_CPU_RESET - 11, Time Group reset CPU";
                    case 12: return "SW_CPU_RESET - 12, Software reset CPU";
                    case 13: return "RTCWDT_CPU_RESET - 13, RTC Watch dog Reset CPU";
                    case 14: return "EXT_CPU_RESET - 14, for APP CPU, reseted by PRO CPU";
                    case 15: return "RTCWDT_BROWN_OUT_RESET - 15, Reset when the vdd voltage is not stable";
                    case 16: return "RTCWDT_RTC_RESET - 16, RTC Watch dog reset digital core and rtc module";
                    default: return "RESET REASON UNKNOWN";
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
            return os << entry.message;
        }

        template<size_t maxSize>
        inline ostream& operator << (ostream& os, const dmesgQueue_t<maxSize>& queue) {
            // if (queue.empty ()) return os; // don't need to check, dmesgQueue is never empty
            return os << queue.back ();
        }

    #endif


    // create a singleton working instance
    inline dmesgQueue_t<DMESG_CIRCULAR_QUEUE_LENGTH> dmesgQueue;

#endif
