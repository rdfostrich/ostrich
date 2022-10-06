#ifndef OSTRICH_VARIABLE_SIZE_INTEGER_H
#define OSTRICH_VARIABLE_SIZE_INTEGER_H

#include <cstdint>
#include <vector>
#include <stdexcept>


inline size_t get_ULEB128_size(uint64_t value) {
    size_t size = 0;
    do {
        value >>= 7;
        size += sizeof(int8_t);
    } while (value);
    return size;
}

inline size_t get_SLEB128_size(int64_t value) {
    size_t size = 0;
    bool more;
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        more = !((((value == 0 ) && ((byte & 0x40) == 0)) || ((value == -1) && ((byte & 0x40) != 0))));
        size += sizeof(int8_t);
    } while (more);
    return size;
}

/**
 * Encode an unsigned integer into a LEB128 value
 * @param value the value to encode
 * @param p the destination buffer (as a vector)
 */
inline void encode_ULEB128(uint64_t value, std::vector<uint8_t>& p) {
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        if (value != 0)
            byte |= 0x80;
        p.push_back(byte);
    } while (value);
}

/**
 * Encode a signed integer into a LEB128 value
 * @param value the value to encode
 * @param p the destination buffer (as a vector)
 */
inline void encode_SLEB128(int64_t value, std::vector<uint8_t>& p) {
    bool is_more;
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        is_more = !((((value == 0 ) && ((byte & 0x40) == 0)) || ((value == -1) && ((byte & 0x40) != 0))));
        if (is_more)
            byte |= 0x80;
        p.push_back(byte);
    } while (is_more);
}



/**
 * Decode an unsigned integer from ULEB128 encoded data
 * @param p the data to decode
 * @param decode_size the amount of bytes decoded
 * @return the decoded value
 */
inline uint64_t decode_ULEB128(const uint8_t *p, size_t* decode_size = nullptr) {
    const uint8_t *old_p = p;
    uint64_t value = 0;
    unsigned shift = 0;
    do {
        uint64_t slice = *p & 0x7f;
        if (shift >= 64 || slice << shift >> shift != slice) {
            throw std::runtime_error("ULEB128 encoded value is too big");
        }
        value += uint64_t(*p & 0x7f) << shift;
        shift += 7;
    } while (*p++ >= 128);
    if (decode_size) {
        *decode_size = (size_t)(p - old_p);
    }
    return value;
}

/**
 * Decode an signed integer from SLEB128 encoded data
 * @param p the data to decode
 * @param decode_size the amount of bytes decoded
 * @return the decoded value
 */
inline int64_t decode_SLEB128(const uint8_t *p, size_t* decode_size = nullptr) {
    int64_t value = 0;
    unsigned offset = 0;
    unsigned type_size = sizeof(int64_t) * 8;
    unsigned shift = 0;
    uint8_t byte;
    do {
        byte = p[offset];
        uint64_t slice = byte & 0x7f;
        if ((shift >= type_size && slice != (value < 0 ? 0x7f : 0x00)) || (shift == type_size-1 && slice != 0 && slice != 0x7f)) {
            throw std::runtime_error("SLEB128 encoded value is too big");
        }
        value |= slice << shift;
        shift += 7;
        offset++;
    } while (byte & 0x80);
    if (shift < type_size && (byte & 0x40)) {
        value |= (-1ULL) << shift;  // sign
    }
    if (decode_size) {
        *decode_size = offset;
    }
    return value;
}

#endif //OSTRICH_VARIABLE_SIZE_INTEGER_H
