# ESP32 Multitasking Network Suite - Arduino Library

Multitasking HTTP, HTTPS, FTP, Telnet servers and thread-safe NTP, HTTP, SMTP clients.

---

## ✅ Key Features

True multitasking architecture - each server runs in its own FreeRTOS task for high concurrency and responsiveness.

Unified modular design - HTTP, HTTPS, FTP, Telnet, NTP, and SMTP components share a consistent API and can be enabled independently.

Thread-safe infrastructure - integrates with thread-safe STL, file system wrapper, ping, and cron libraries for reliable parallel operation.

Lightweight and ESP32-optimized - minimal overhead, designed specifically for Arduino-based ESP32 environments.

HTTP server - supports dynamic handlers, static content, file content (including .gz) and WebSockets.

HTTPS server - HTTP server using Arduino wolfSSL Library (https://github.com/wolfSSL/Arduino-wolfSSL) as the TLS transport layer.

FTP server - active or pasive file transfers with safe access to ESP32 file systems.

Telnet server - remote debugging, logging, and interactive console access with built-in commands (like ping, ls, text editor, ...).

NTP client - time synchronization with configurable servers.

SMTP client - send emails directly from the ESP32 without external services.

Easy integration - drop-in components with minimal setup and clear separation of concerns.

---
