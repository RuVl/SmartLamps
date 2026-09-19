#include "Psk.h"

#ifndef ESP32
#include <bearssl/bearssl_hash.h>
#include <bearssl/bearssl_hmac.h>
#endif

namespace net::psk
{
    namespace
    {
        String g_key;
        bool g_busy = false;

#ifndef ESP32
        constexpr uint16_t kRounds = 4096;
        constexpr uint8_t kBlocks = 2; // 2 x 20 bytes of SHA-1 cover the 32-byte key
        // Rounds per step(): about 3 ms on the 80 MHz core, well inside a frame.
        constexpr uint16_t kSlice = 96;

        String g_ssid;
        br_hmac_key_context g_kc;
        uint8_t g_u[20]; // U(n) of the running block
        uint8_t g_t[kBlocks * 20]; // T(1) || T(2), the key itself
        uint8_t g_block = 0;
        uint16_t g_round = 0;

        void hmac(const void* data, size_t len, uint8_t* out)
        {
            br_hmac_context hc;
            br_hmac_init(&hc, &g_kc, 0);
            br_hmac_update(&hc, data, len);
            br_hmac_out(&hc, out);
        }

        void beginBlock()
        {
            // U(1) = PRF(pass, ssid || INT(block + 1))
            br_hmac_context hc;
            br_hmac_init(&hc, &g_kc, 0);
            br_hmac_update(&hc, g_ssid.c_str(), g_ssid.length());
            const uint8_t index[4] = {0, 0, 0, uint8_t(g_block + 1)};
            br_hmac_update(&hc, index, sizeof(index));
            br_hmac_out(&hc, g_u);
            memcpy(g_t + g_block * 20, g_u, 20);
            g_round = 1;
        }
#endif
    }

    uint32_t id(const String& ssid, const String& pass)
    {
        // FNV-1a over "ssid\0pass": the separator keeps ("ab","c") and ("a","bc") apart.
        uint32_t h = 2166136261u;
        auto mix = [&h](const String& s)
        {
            for (size_t i = 0; i <= s.length(); ++i) // the terminator included
            {
                h ^= uint8_t(s[i]);
                h *= 16777619u;
            }
        };
        mix(ssid);
        mix(pass);
        return h;
    }

    void start(const String& ssid, const String& pass)
    {
#ifdef ESP32
        (void)ssid;
        g_key = pass;
        g_busy = true;
#else
        g_ssid = ssid;
        br_hmac_key_init(&g_kc, &br_sha1_vtable, pass.c_str(), pass.length());
        g_block = 0;
        beginBlock();
        g_busy = true;
#endif
    }

    bool busy() { return g_busy; }

    bool step()
    {
        if (!g_busy) return false;
#ifdef ESP32
        g_busy = false;
        return true;
#else
        uint16_t budget = kSlice;
        while (budget > 0)
        {
            if (g_round >= kRounds)
            {
                if (++g_block >= kBlocks)
                {
                    static const char hex[] = "0123456789abcdef";
                    g_key = "";
                    g_key.reserve(64);
                    for (uint8_t b : g_t)
                    {
                        g_key += hex[b >> 4];
                        g_key += hex[b & 15];
                    }
                    g_busy = false;
                    return true;
                }
                beginBlock();
                continue;
            }
            // U(n) = PRF(pass, U(n-1)); T ^= U(n)
            hmac(g_u, sizeof(g_u), g_u);
            uint8_t* t = g_t + g_block * 20;
            for (uint8_t i = 0; i < 20; ++i) t[i] ^= g_u[i];
            ++g_round;
            --budget;
        }
        return false;
#endif
    }

    const String& key() { return g_key; }
}
