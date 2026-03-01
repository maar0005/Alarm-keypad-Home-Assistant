#pragma once
// =============================================================================
// alarm_keypad_security.h
// Security helpers for alarm-keypad-factory firmware
//
// Uses SHA-256 (mbedTLS, bundled with ESP-IDF and Arduino for ESP32).
// HA's Jinja2 template engine supports {{ value | hash('sha256') }} natively,
// so no Python or external tools are needed on the HA side.
// =============================================================================

#include <string>
#include <cstdio>
#include <cctype>
#include "mbedtls/sha256.h"
#include "esp_random.h"   // hardware RNG on ESP32

// -----------------------------------------------------------------------------
// sha256_hex(input) → 64-char lowercase hex string
//
// The device computes SHA-256(device_salt + entered_pin) and sends only that
// hash to HA.  The raw PIN never leaves the device.
//
// HA verifies with:
//   {{ (device_salt + alarm_pin) | hash('sha256') == received_hash }}
// — no Python or external tools required.
// -----------------------------------------------------------------------------
inline std::string sha256_hex(const std::string& input) {
    uint8_t raw[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, /*is_224=*/0);
    mbedtls_sha256_update(&ctx,
        reinterpret_cast<const unsigned char*>(input.data()), input.size());
    mbedtls_sha256_finish(&ctx, raw);
    mbedtls_sha256_free(&ctx);

    char hex[65];
    for (int i = 0; i < 32; i++) sprintf(hex + 2 * i, "%02x", raw[i]);
    hex[64] = '\0';
    return std::string(hex);
}

// -----------------------------------------------------------------------------
// generate_device_salt() → 32-char hex string (16 bytes from hardware RNG)
// Called once on first boot; stored in NVS via a restore_value global.
// -----------------------------------------------------------------------------
inline std::string generate_device_salt() {
    uint32_t r[4];
    for (int i = 0; i < 4; i++) r[i] = esp_random();
    char buf[33];
    for (int i = 0; i < 4; i++) sprintf(buf + i * 8, "%08x", r[i]);
    buf[32] = '\0';
    return std::string(buf);
}

// -----------------------------------------------------------------------------
// derive_display_code(salt) → 6-char uppercase pairing code shown on LCD.
// Also exposed as a HA sensor so it can be copy-pasted into the pairing field.
// -----------------------------------------------------------------------------
inline std::string derive_display_code(const std::string& salt) {
    if (salt.size() < 6) return "??????";
    std::string code = salt.substr(0, 6);
    for (char& c : code) c = static_cast<char>(toupper(c));
    return code;
}
