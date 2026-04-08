# ESP32 Multitasking Network Suite

Multitasking HTTP, FTP, Telnet servers and thread-safe NTP, HTTP, SMTP clients.

---

## ✅ Key Features

True multitasking architecture - each server runs in its own FreeRTOS task for high concurrency and responsiveness.

Unified modular design - HTTP, FTP, Telnet, NTP, and SMTP components share a consistent API and can be enabled independently.

Thread-safe infrastructure - integrates with thread-safe STL, file system wrapper, ping, and cron libraries for reliable parallel operation.

Lightweight and ESP32-optimized - minimal overhead, designed specifically for Arduino-based ESP32 environments.

HTTP server - supports dynamic handlers, static content, file content (including .gz), REST-style endpoints and WebSockets.

FTP server - active or pasive file transfers with safe access to ESP32 file systems.

Telnet server - remote debugging, logging, and interactive console access with built-in commands (like ping, ls, text editor, ...).

NTP client - time synchronization with configurable servers.

SMTP client - send emails directly from the ESP32 without external services.

Non-blocking operation - all protocols designed to avoid blocking the main loop.

Easy integration - drop-in components with minimal setup and clear separation of concerns.

---

## ✅ Why use multitasking servers instead of single-tasked ones

Because it makes development significantly easier.

The programmer must ensure thread-safety and reentrancy, but does not need to manage the state machines of multiple simultaneous clients - each client is handled independently in its own task.
 
