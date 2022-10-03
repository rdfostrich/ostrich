#ifndef OSTRICH_VARIABLE_SIZE_INTEGER_H
#define OSTRICH_VARIABLE_SIZE_INTEGER_H

#include <cstdint>
#include <vector>
#include <stdexcept>


template <class I>
inline size_t get_ULEB128_size(I value) {
    size_t size = 0;
    do {
        value >>= 7;
        size += sizeof(int8_t);
    } while (value);
    return size;
}

template <class I>
inline size_t get_SLEB128_size(I value) {
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
    } while (value);
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

/**
 * Decode an unsigned integer from ULEB128 encoded data
 * @tparam I the integer type
 * @param p the data to decode
 * @param decode_size the amount of bytes decoded
 * @return the decoded value
 */
template <class I>
inline I decode_ULEB128(const uint8_t *p, size_t* decode_size = nullptr) {
    const uint8_t *old_p = p;
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
    if (decode_size) {
        *decode_size = (size_t)(p - old_p);
    }
    return value;
}

/**
 * Decode an signed integer from SLEB128 encoded data
 * @tparam I the integer type
 * @param p the data to decode
 * @param decode_size the amount of bytes decoded
 * @return the decoded value
 */
template <class I>
inline I decode_SLEB128(const uint8_t *p, size_t* decode_size = nullptr) {
    const uint8_t *old_p = p;
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
    if (shift < (sizeof(I)*8) && (byte & 0x40))
        value |= (-1ULL) << shift;
    if (decode_size) {
        *decode_size = (size_t)(p - old_p);
    }
    return value;
}

#endif //OSTRICH_VARIABLE_SIZE_INTEGER_H
