/* --------------------------------------------------------------------------

    WolfSSL user_settings.h example file

    This WolfSSL user_settings.h configuration is optimized for ESP32-WROOM 
    modules without PSRAM and is suitable for TLS certificates and keys based
    on elliptic curves (ECC), especially secp256r1. 

    It enables the required ECC features, disables unsupported curves 
    (Curve25519/Ed25519), and minimizes RAM usage through static memory allocation.

    The configuration is ideal for HTTPS, WSS, MQTT‑TLS, and other secure IoT 
    applications running on ESP32 with Arduino Core or ESP-IDF.

    --------------------------------------------------------------------------

    CreateSelfSignedCertificateAndKey.cmd example file

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

   -------------------------------------------------------------------------- */

#ifndef WOLFSSL_USER_SETTINGS_H
#define WOLFSSL_USER_SETTINGS_H


/* --------------------------------------------------------------------------
   PLATFORM DEFINITIONS
   -------------------------------------------------------------------------- */

/* Tell WolfSSL that we are running on ESP32 */
#define WOLFSSL_ESP32

/* Optimize for the ESP32-WROOM module */
#define WOLFSSL_ESP32_WROOM

/* FreeRTOS is the underlying scheduler */
#define FREERTOS

/* Arduino framework compatibility (ESP32 Arduino Core) */
#define WOLFSSL_ARDUINO



/* --------------------------------------------------------------------------
   HARDWARE CRYPTO ACCELERATION
   -------------------------------------------------------------------------- */

/*
   ESP32 provides hardware acceleration for AES, SHA, RSA, etc.
   However, the SHA hardware engine may conflict with Arduino SHA libraries.
   If you encounter SHA-related compile errors, keep this define enabled.
*/
// #define WOLFSSL_ESP32_CRYPT
#define NO_WOLFSSL_ESP32_CRYPT_HASH



/* --------------------------------------------------------------------------
   TLS VERSION AND CRYPTOGRAPHY
   -------------------------------------------------------------------------- */

/*
   TLS 1.3 is faster and better suited for multitasking environments.
*/
#define WOLFSSL_TLS13

/* Required algorithms for TLS 1.3 */
#define HAVE_AESGCM
#define HAVE_HKDF
#define HAVE_FFDHE_2048

/* RSA-PSS support (required for RSA certificates in TLS 1.3) */
#define WC_RSA_PSS
#define HAVE_HMAC
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES



/* --------------------------------------------------------------------------
   MEMORY OPTIMIZATION (for devices without PSRAM)
   -------------------------------------------------------------------------- */

/*
   Static memory reduces fragmentation and improves stability
   on systems with limited RAM.
*/
#define WOLFSSL_STATIC_MEMORY

/* Faster big‑integer math implementation */
#define USE_FAST_MATH

/* Protection against timing attacks */
#define TFM_TIMING_RESISTANT



/* --------------------------------------------------------------------------
   REMOVE UNUSED FEATURES (smaller binary size)
   -------------------------------------------------------------------------- */

/* Disable old TLS versions (TLS 1.0 / TLS 1.1) */
#define NO_OLD_TLS

/* Disable PSK authentication (not used in most IoT devices) */
#define NO_PSK

/* Remove obsolete hash algorithms */
#define NO_MD4

/* Remove password‑based key derivation (not needed here) */
#define NO_PWDBASED

/* Disable SNI if not required */
#define NO_SNI



/* --------------------------------------------------------------------------
   OVERRIDE: Switch TLS 1.3 → TLS 1.2
   -------------------------------------------------------------------------- */

/*
   Example of overriding previous settings.
   If you need TLS 1.2 for compatibility with older clients,
   simply undefine TLS 1.3 and enable TLS 1.2.
*/
// #undef WOLFSSL_TLS13
// #define WOLFSSL_TLS12



/* --------------------------------------------------------------------------
   REQUIRED: Tell WolfSSL to use this user_settings.h
   -------------------------------------------------------------------------- */
#define WOLFSSL_USER_SETTINGS

#endif /* WOLFSSL_USER_SETTINGS_H */
