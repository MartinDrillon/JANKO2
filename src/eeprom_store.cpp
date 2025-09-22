#include "eeprom_store.h"
#include <EEPROM.h>

namespace {
    struct Header {
        uint32_t magic;   // 'J2TH'
        uint16_t version; // 1
        uint16_t nMux;
        uint16_t nCh;
        uint32_t crc;     // CRC32 of payload (low+high arrays)
    } __attribute__((packed));

    constexpr uint32_t kMagic = 0x4A325448; // 'J2TH'
    constexpr uint16_t kVersion = 1;

    uint32_t crc32_update(uint32_t crc, uint8_t data) {
        crc ^= data;
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ (0xEDB88320UL & (-(int)(crc & 1)));
        }
        return crc;
    }

    uint32_t crc32_buf(const uint8_t* buf, size_t len) {
        uint32_t crc = 0xFFFFFFFFu;
        for (size_t i = 0; i < len; ++i) crc = crc32_update(crc, buf[i]);
        return ~crc;
    }
}

namespace EepromStore {
    static size_t payloadSize() {
        return (size_t)N_MUX * (size_t)N_CH * sizeof(uint16_t) * 2; // low + high
    }

    bool load(uint16_t low[N_MUX][N_CH], uint16_t high[N_MUX][N_CH]) {
        Header hdr{};
        EEPROM.get(0, hdr);
        if (hdr.magic != kMagic || hdr.version != kVersion || hdr.nMux != N_MUX || hdr.nCh != N_CH) {
            return false;
        }
        size_t off = sizeof(Header);
        size_t psize = payloadSize();
        // Read into a temp buffer to verify CRC
        uint8_t* buf = (uint8_t*)malloc(psize);
        if (!buf) return false;
        for (size_t i = 0; i < psize; ++i) {
            buf[i] = EEPROM.read(off + i);
        }
        uint32_t crc = crc32_buf(buf, psize);
        if (crc != hdr.crc) {
            free(buf);
            return false;
        }
        // Unpack
        size_t idx = 0;
        for (uint8_t m = 0; m < N_MUX; ++m) {
            for (uint8_t c = 0; c < N_CH; ++c) {
                uint16_t v = (uint16_t)buf[idx] | ((uint16_t)buf[idx+1] << 8);
                low[m][c] = v; idx += 2;
            }
        }
        for (uint8_t m = 0; m < N_MUX; ++m) {
            for (uint8_t c = 0; c < N_CH; ++c) {
                uint16_t v = (uint16_t)buf[idx] | ((uint16_t)buf[idx+1] << 8);
                high[m][c] = v; idx += 2;
            }
        }
        free(buf);
        return true;
    }

    void save(const uint16_t low[N_MUX][N_CH], const uint16_t high[N_MUX][N_CH]) {
        Header hdr{};
        hdr.magic = kMagic;
        hdr.version = kVersion;
        hdr.nMux = N_MUX;
        hdr.nCh = N_CH;
        size_t psize = payloadSize();
        uint8_t* buf = (uint8_t*)malloc(psize);
        if (!buf) return;
        size_t idx = 0;
        for (uint8_t m = 0; m < N_MUX; ++m) {
            for (uint8_t c = 0; c < N_CH; ++c) {
                uint16_t v = low[m][c];
                buf[idx++] = (uint8_t)(v & 0xFF);
                buf[idx++] = (uint8_t)(v >> 8);
            }
        }
        for (uint8_t m = 0; m < N_MUX; ++m) {
            for (uint8_t c = 0; c < N_CH; ++c) {
                uint16_t v = high[m][c];
                buf[idx++] = (uint8_t)(v & 0xFF);
                buf[idx++] = (uint8_t)(v >> 8);
            }
        }
        hdr.crc = crc32_buf(buf, psize);
        // Write header
        EEPROM.put(0, hdr);
        // Write payload
        size_t off = sizeof(Header);
        for (size_t i = 0; i < psize; ++i) {
            EEPROM.write(off + i, buf[i]);
        }
    // Teensy EEPROM writes are applied immediately; no commit required
        free(buf);
    }
}
