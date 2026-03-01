#pragma once
// =============================================================================
// alarm_keypad_security.h
// Security helpers for alarm-keypad-factory firmware
//
// SECURITY MODEL
// ──────────────
// Each device generates a unique 32-byte (256-bit) key at first boot using the
// ESP32 hardware RNG.  The key is:
//
//   1. Stored in NVS (survives reboots, wiped only on factory reset).
//   2. Used as the ESPHome API encryption PSK — all traffic between the device
//      and HA is ciphertext (Noise_NNpsk0 protocol).
//   3. Used as the HMAC key for PIN hashing — SHA-256(key + PIN).  The raw
//      PIN never leaves the device; only the hash is sent to HA.
//
// ONE KEY, TWO JOBS — the user copies it once from the device web page and
// pastes it in two places: HA API pairing, then the Blueprint.
//
// HA verifies the PIN hash with: {{ (device_key + alarm_pin) | hash('sha256') }}
// No Python or external tools required.
// =============================================================================

#include <string>
#include <array>
#include <vector>
#include <cstdio>
#include <cctype>
#include <cstring>
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"
#include "esp_random.h"   // hardware RNG on ESP32
#include "nvs_flash.h"
#include "nvs.h"

// NVS namespace + key used to persist the device key independently of
// ESPHome's own preferences partition so we can read it at boot priority 400
// (before the API server initialises).
static const char* KP_NVS_NS  = "kp_key";
static const char* KP_NVS_KEY = "dev_key";

// -----------------------------------------------------------------------------
// sha256_hex(input) → 64-char lowercase hex string
//
// Device computes: SHA-256(device_key_base64 + entered_pin)
// HA verifies with: {{ (device_key + alarm_pin) | hash('sha256') }}
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
// generate_device_key() → 32 random bytes encoded as base64 (44 chars)
//
// Uses the ESP32 hardware RNG (esp_fill_random).  Called once at first boot;
// the result is saved to NVS by ensure_device_key().
// -----------------------------------------------------------------------------
inline std::string generate_device_key() {
    uint8_t raw[32];
    esp_fill_random(raw, sizeof(raw));

    unsigned char b64[64] = {};
    size_t out_len = 0;
    mbedtls_base64_encode(b64, sizeof(b64), &out_len, raw, sizeof(raw));
    return std::string(reinterpret_cast<char*>(b64), out_len);
}

// -----------------------------------------------------------------------------
// key_to_psk(b64_key) → std::array<uint8_t, 32>
//
// Decodes the base64 key string into the raw byte array expected by
// ESPHome's APIServer::set_noise_psk().
// Returns a zeroed array on decode failure (caller should check size match).
// -----------------------------------------------------------------------------
inline std::array<uint8_t, 32> key_to_psk(const std::string& b64_key) {
    std::array<uint8_t, 32> psk{};
    size_t out_len = 0;
    int ret = mbedtls_base64_decode(
        psk.data(), psk.size(), &out_len,
        reinterpret_cast<const unsigned char*>(b64_key.c_str()), b64_key.size());
    if (ret != 0 || out_len != 32) psk.fill(0);
    return psk;
}

// -----------------------------------------------------------------------------
// NVS helpers — read/write from our own namespace so we can access the key
// before ESPHome's preference layer is fully initialised.
// -----------------------------------------------------------------------------
inline std::string nvs_load_device_key() {
    nvs_handle_t h;
    if (nvs_open(KP_NVS_NS, NVS_READONLY, &h) != ESP_OK) return "";
    char buf[64] = {};
    size_t len = sizeof(buf);
    esp_err_t err = nvs_get_str(h, KP_NVS_KEY, buf, &len);
    nvs_close(h);
    if (err != ESP_OK) return "";
    return std::string(buf);
}

inline void nvs_save_device_key(const std::string& key) {
    nvs_handle_t h;
    if (nvs_open(KP_NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_str(h, KP_NVS_KEY, key.c_str());
    nvs_commit(h);
    nvs_close(h);
}

// -----------------------------------------------------------------------------
// ensure_device_key() → load from NVS; generate & save if missing/invalid.
// Returns the 44-char base64 key string on success, empty string on failure.
// -----------------------------------------------------------------------------
inline std::string ensure_device_key() {
    std::string key = nvs_load_device_key();
    if (key.size() == 44) return key;          // already have a valid key

    key = generate_device_key();
    if (key.size() != 44) return "";           // RNG or base64 failure

    nvs_save_device_key(key);
    return key;
}

// -----------------------------------------------------------------------------
// derive_display_code(key) → 6-char uppercase device identifier shown on LCD.
//
// Picks the first 6 alphanumeric characters from the base64 key and
// uppercases them.  Used to visually identify a device (not a security gate).
// -----------------------------------------------------------------------------
inline std::string derive_display_code(const std::string& key) {
    std::string code;
    for (unsigned char c : key) {
        if (isalnum(c)) {
            code += static_cast<char>(toupper(c));
            if (code.size() == 6) break;
        }
    }
    while (code.size() < 6) code += '?';
    return code;
}
