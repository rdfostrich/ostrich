#ifndef OSTRICH_VARIABLE_SIZE_INTEGER_H
#define OSTRICH_VARIABLE_SIZE_INTEGER_H

#include <cstdint>
#include <vector>
#include <stdexcept>

/**
 * Encode an unsigned integer into a LEB128 value
 * @tparam I the integer type
 * @param value the value to encode
 * @param p the destination buffer (as a vector)
 */
template <class I>
inline void encode_ULEB128(I value, std::vector<uint8_t>& p) {
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        if (value != 0)
            byte |= 0x80;
        p.push_back(byte);
    } while (value != 0);
}

/**
 * Encode a signed integer into a LEB128 value
 * @tparam I the integer type
 * @param value the value to encode
 * @param p the destination buffer (as a vector)
 */
template <class I>
inline void encode_SLEB128(I value, std::vector<uint8_t>& p) {
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


template <class I>
inline I decode_ULEB128(const uint8_t *p) {
    I value = 0;
    unsigned shift = 0;
    do {
        I slice = *p & 0x7f;
        if (shift >= (sizeof(I)*8) || slice << shift >> shift != slice) {
            throw std::runtime_error("ULEB128 encoded value is too big");
        }
        value += I(*p & 0x7f) << shift;
        shift += 7;
    } while (*p++ >= 128);
    return value;
}


template <class I>
inline I decode_SLEB128(const uint8_t *p) {
    I value = 0;
    unsigned shift = 0;
    uint8_t byte;
    do {
        byte = *p;
        I slice = byte & 0x7f;
        if ((shift >= (sizeof(I)*8) && slice != (value < 0 ? 0x7f : 0x00)) || (shift == 63 && slice != 0 && slice != 0x7f)) {
            throw std::runtime_error("SLEB128 encoded value is too big");
        }
        value |= slice << shift;
        shift += 7;
        ++p;
    } while (byte >= 128);
    // Sign extend negative numbers if needed.
    if (shift < (sizeof(I)*8) && (byte & 0x40))
        value |= (-1ULL) << shift;
    return value;
}

#endif //OSTRICH_VARIABLE_SIZE_INTEGER_H
