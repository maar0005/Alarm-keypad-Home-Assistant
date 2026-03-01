#pragma once
// =============================================================================
// alarm_keypad_security.h
// Security helpers for alarm-keypad-factory firmware
//
// Included via esphome: includes: [alarm_keypad_security.h]
// Uses mbedTLS which is bundled with both esp-idf and Arduino (ESP32).
// =============================================================================

#include <string>
#include <cstdio>
#include <cctype>
#include "mbedtls/md.h"
#include "esp_random.h"   // esp_random() — hardware RNG on ESP32

// -----------------------------------------------------------------------------
// hmac_sha256_hex(key, message) → 64-char lowercase hex string
//
// The PIN entered on the keypad is NEVER transmitted.
// Only HMAC-SHA256(PIN, device_salt) is sent to HA.
// Without the device_salt, the HMAC cannot be reversed to recover the PIN.
// -----------------------------------------------------------------------------
inline std::string hmac_sha256_hex(const std::string& key, const std::string& msg) {
    uint8_t raw[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), /*hmac=*/1);
    mbedtls_md_hmac_starts(&ctx,
        reinterpret_cast<const unsigned char*>(key.data()), key.size());
    mbedtls_md_hmac_update(&ctx,
        reinterpret_cast<const unsigned char*>(msg.data()), msg.size());
    mbedtls_md_hmac_finish(&ctx, raw);
    mbedtls_md_free(&ctx);

    char hex[65];
    for (int i = 0; i < 32; i++) sprintf(hex + 2 * i, "%02x", raw[i]);
    hex[64] = '\0';
    return std::string(hex);
}

// -----------------------------------------------------------------------------
// generate_device_salt() → 32-char hex string (16 bytes from hardware RNG)
//
// Called once on first boot.  Stored in NVS via a globals restore_value.
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
// derive_display_code(salt) → 6-char uppercase pairing code
//
// Shown on the LCD in provisioning mode.  HA must send this code back to
// the device to complete pairing.  Physically verifies the user is present.
// -----------------------------------------------------------------------------
inline std::string derive_display_code(const std::string& salt) {
    if (salt.size() < 6) return "??????";
    std::string code = salt.substr(0, 6);
    for (char& c : code) c = static_cast<char>(toupper(c));
    return code;
}
