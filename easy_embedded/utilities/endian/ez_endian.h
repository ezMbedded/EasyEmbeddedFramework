#ifndef EZ_ENDIAN_H
#define EZ_ENDIAN_H

#include <stdint.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define EZHTON16(x) bswap16(x)
#define EZHTON32(x) bswap32(x)
#define EZHTON64(x) bswap64(x)
#define EZNTOH16(x) bswap16(x)
#define EZNTOH32(x) bswap32(x)
#define EZNTOH64(x) bswap64(x)
#else
#define EZHTON16(x)
#define EZHTON32(x)
#define EZHTON64(x)
#define EZNTOH16(x)
#define EZNTOH32(x)
#define EZNTOH64(x)
#endif


static inline uint16_t bswap16(uint16_t x) {
    return (uint16_t)((x << 8) | (x >> 8));
}

static inline uint32_t bswap32(uint32_t x) {
    return  ((x & 0x000000FFU) << 24) |
            ((x & 0x0000FF00U) <<  8) |
            ((x & 0x00FF0000U) >>  8) |
            ((x & 0xFF000000U) >> 24);
}

static inline uint64_t bswap64(uint64_t x) {
    return  ((x & 0x00000000000000FFULL) << 56) |
            ((x & 0x000000000000FF00ULL) << 40) |
            ((x & 0x0000000000FF0000ULL) << 24) |
            ((x & 0x00000000FF000000ULL) <<  8) |
            ((x & 0x000000FF00000000ULL) >>  8) |
            ((x & 0x0000FF0000000000ULL) >> 24) |
            ((x & 0x00FF000000000000ULL) >> 40) |
            ((x & 0xFF00000000000000ULL) >> 56);
}

#endif
