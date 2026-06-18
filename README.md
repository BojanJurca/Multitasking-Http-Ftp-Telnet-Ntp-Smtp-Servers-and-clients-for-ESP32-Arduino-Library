# ESP32 Multitasking Network Suite - Arduino Library

Multitasking HTTP, HTTPS, FTP, Telnet servers and thread-safe NTP, HTTP, HTTPS, SMTP clients.

---

## ✅ Key Features

True multitasking architecture - each server runs in its own FreeRTOS task for high concurrency and responsiveness.

Unified modular design - HTTP, HTTPS, FTP, Telnet, NTP, and SMTP components share a consistent API and can be enabled independently.

Thread-safe infrastructure - integrates with thread-safe STL, file system wrapper, ping, and cron libraries for reliable parallel operation.

Lightweight and ESP32-optimized - minimal overhead, designed specifically for Arduino-based ESP32 environments.

HTTP server - supports dynamic handlers, static content, file content (including .gz) and WebSockets.

HTTPS server - HTTP server using Arduino wolfSSL Library (ensure that .../libraries/wolfssl/src/user_settings.h is configured correctly for your board) as the TLS transport layer.

FTP server - active or pasive file transfers with safe access to ESP32 file systems.

Telnet server - remote debugging, logging, and interactive console access with built-in commands (like ping, ls, text editor, ...).

NTP client - time synchronization with configurable servers.

HTTP client - retrieve data from external servers.

HTTPS client – secure HTTP client using the Arduino WolfSSL library (ensure that .../libraries/wolfssl/src/user_settings.h is configured correctly for your board). TLS encryption is supported, but certificate/key verification is currently not implemented.
SMTP client - send emails directly from the ESP32.

Easy integration - drop-in components with minimal setup and clear separation of concerns.

---
