/* ==========================================================================
   UNIVERSAL WolfSSL user_settings.h for at least ESP32-WROOM and ESP32-S2
   Compatible in BOTH directions (client ↔ server)
   ECC-only (secp256r1), TLS 1.2 + TLS 1.3, AES-GCM, no RSA.


   Certificate generation example:

   @echo off
   REM ---------------------------------------------------------------------------
   REM  Example: Generate an ECC-based self-signed TLS certificate for ESP32
   REM  This script creates:
   REM     - ECC private key (prime256v1 / secp256r1)
   REM     - Self-signed X.509 certificate (PEM)
   REM     - DER-encoded key and certificate (required by many ESP32 libraries)
   REM
   REM  Curve prime256v1 (secp256r1) is recommended for ESP32 because:
   REM     - It is fast
   REM     - Uses minimal RAM
   REM     - Fully supported by WolfSSL and mbedTLS
   REM ---------------------------------------------------------------------------


   REM ---------------------------------------------------------------------------
   REM 1) Generate a new ECC private key + self-signed certificate (PEM format)
   REM    -newkey ec                     → create a new elliptic-curve key
   REM    -pkeyopt ec_paramgen_curve:prime256v1 → use secp256r1 curve
   REM    -x509                          → output a self-signed certificate
   REM    -nodes                         → do not encrypt the private key
   REM    -days 3650                     → certificate validity (10 years)
   REM    -subj                          → certificate subject fields
   REM ---------------------------------------------------------------------------

   "C:\Program Files\OpenSSL-Win64\bin\openssl" req ^
   -new -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 ^
   -days 3650 -nodes -x509 ^
   -subj "/C=SI/ST=Slovenia/L=Ljubljana/O=ESP32/CN=jurca.dyn.ts.si" ^
   -keyout server-key.pem ^
   -out server-cert.pem

   REM ---------------------------------------------------------------------------
   REM 2) Convert the ECC private key from PEM → DER
   REM    ESP32/WolfSSL often require DER format for embedded TLS
   REM ---------------------------------------------------------------------------

   "C:\Program Files\OpenSSL-Win64\bin\openssl" ec ^
   -in server-key.pem ^
   -outform DER ^
   -out server-key.der

   REM ---------------------------------------------------------------------------
   REM 3) Convert the certificate from PEM → DER
   REM    DER is a compact binary format suitable for microcontrollers
   REM ---------------------------------------------------------------------------

   "C:\Program Files\OpenSSL-Win64\bin\openssl" x509 ^
   -in server-cert.pem ^
   -outform DER ^
   -out server-cert.der

   REM ---------------------------------------------------------------------------
   REM  Done!
   REM  You now have:
   REM     server-key.pem   → ECC private key (PEM)
   REM     server-cert.pem  → Self-signed certificate (PEM)
   REM     server-key.der   → ECC private key (DER)
   REM     server-cert.der  → Certificate (DER)
   REM
   REM  These files are ready for use with ESP32 HTTPS/TLS servers.
   REM ---------------------------------------------------------------------------

   echo ECC certificate and key generation completed successfully.
   pause

   ========================================================================== */


#ifndef WOLFSSL_USER_SETTINGS_H
#define WOLFSSL_USER_SETTINGS_H


/* --------------------------------------------------------------------------
   DEBUGGING
   -------------------------------------------------------------------------- */

// #define DEBUG_WOLFSSL
// Besides #defining DEBUG_WOLFSSL the wolfSSL_Debugging_ON (); function needs to be called 


/* --------------------------------------------------------------------------
   PLATFORM
   -------------------------------------------------------------------------- */
#define WOLFSSL_ARDUINO
#define WOLFSSL_ESP32
#define FREERTOS


/* Disable hardware crypto everywhere (WROOM and S2 do not support it) */
#define NO_WOLFSSL_ESP32_CRYPT_HASH


/* --------------------------------------------------------------------------
   TLS VERSIONS
   -------------------------------------------------------------------------- */
#define WOLFSSL_TLS13
#define NO_OLD_TLS
/* DO NOT disable TLS 1.2 — ESP32-S2 needs fallback */


/* --------------------------------------------------------------------------
   TLS EXTENSIONS (REQUIRED for TLS 1.3)
   -------------------------------------------------------------------------- */
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_HKDF
// #define HAVE_SNI        /* allow SNI from browsers */


/* --------------------------------------------------------------------------
   ECC CONFIGURATION
   -------------------------------------------------------------------------- */
#define HAVE_ECC
#define ECC_USER_CURVES
#undef NO_ECC256
#define HAVE_ECDH
#define HAVE_ECC256
#define ECC_SHAMIR
#define ECC_TIMING_RESISTANT


/* --------------------------------------------------------------------------
   SYMMETRIC CRYPTO
   -------------------------------------------------------------------------- */
#define HAVE_AESGCM
#define HAVE_HMAC
/* SHA256 required */


/* --------------------------------------------------------------------------
   RSA DISABLED (ECC-only)
   -------------------------------------------------------------------------- */
#define NO_RSA


/* --------------------------------------------------------------------------
   MEMORY CONFIGURATION
   -------------------------------------------------------------------------- */
#define USE_FAST_MATH
#define TFM_TIMING_RESISTANT

/* TLS 1.3 handshake requires >2 KB record size */
#undef RECORD_SIZE
#define RECORD_SIZE 4096
#define WOLFSSL_MAX_FRAGMENT_SIZE 4096


/* ECC big integer limit — required for DER parsing */
#undef FP_MAX_BITS
#define FP_MAX_BITS 4096


/* Static memory disabled */
// #define WOLFSSL_STATIC_MEMORY


/* --------------------------------------------------------------------------
   OPTIONAL MEMORY SAVING (SAFE)
   -------------------------------------------------------------------------- */
#define NO_SESSION_CACHE
#define NO_PSK
#define NO_MD4
#define NO_PWDBASED
#define NO_ERROR_STRINGS   /* optional */


/* --------------------------------------------------------------------------
   UNSAFE / INCOMPATIBLE FEATURES (MUST BE DISABLED)
   -------------------------------------------------------------------------- */
#undef SINGLE_THREADED
#undef WOLFSSL_DYNAMIC_BUFFERS
#undef ALT_ECC_SIZE


/* --------------------------------------------------------------------------
   END
   -------------------------------------------------------------------------- */

#endif /* WOLFSSL_USER_SETTINGS_H */
