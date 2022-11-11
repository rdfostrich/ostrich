#include <gtest/gtest.h>

#include "../../../main/cpp/patch/variable_size_integer.h"


/**
 * It is useful to run those tests with ASAN/Valgrind to check if nothing silly happens with memory
 * even if the results are correct
 */

TEST(VariableSizeInteger, SignedMaxInt32) {
    int32_t max = std::numeric_limits<int32_t>::max();
    int32_t it = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (int32_t i=max; i>(max-it); i--) {
        encode_SLEB128(i, buffer);
        int32_t i2 = decode_SLEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_SLEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, SignedMinInt32) {
    int32_t min = std::numeric_limits<int>::min();
    int32_t it = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (int32_t i=min; i<(min+it); i++) {
        encode_SLEB128(i, buffer);
        int32_t i2 = decode_SLEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_SLEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, SignedMiddleInt32) {
    int32_t min = -10000;
    int32_t max = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (int32_t i=min; i<max; i++) {
        encode_SLEB128(i, buffer);
        int32_t i2 = decode_SLEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_SLEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, SignedMaxInt64) {
    int64_t max = std::numeric_limits<int32_t>::max();
    int64_t it = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (int64_t i=max; i>(max-it); i--) {
        encode_SLEB128(i, buffer);
        int64_t i2 = decode_SLEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_SLEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, SignedMinInt64) {
    int64_t min = std::numeric_limits<int>::min();
    int64_t it = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (int64_t i=min; i<(min+it); i++) {
        encode_SLEB128(i, buffer);
        int64_t i2 = decode_SLEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_SLEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, SignedMiddleInt64) {
    int64_t min = -10000;
    int64_t max = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (int64_t i=min; i<max; i++) {
        encode_SLEB128(i, buffer);
        int64_t i2 = decode_SLEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_SLEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, UnsignedMaxInt32) {
    uint32_t max = std::numeric_limits<uint32_t>::max();
    uint32_t it = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (uint32_t i=max; i>(max-it); i--) {
        encode_ULEB128(i, buffer);
        uint32_t i2 = decode_ULEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_ULEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, UnsignedMinInt32) {
    uint32_t it = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (uint32_t i=0; i<it; i++) {
        encode_ULEB128(i, buffer);
        uint32_t i2 = decode_ULEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_ULEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, UnsignedMiddleInt32) {
    uint32_t max = std::numeric_limits<uint32_t>::max();
    uint32_t start = (max/2) - 10000;
    uint32_t end = (max/2) + 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (uint32_t i=start; i<end; i++) {
        encode_ULEB128(i, buffer);
        uint32_t i2 = decode_ULEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_ULEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, UnsignedMaxInt64) {
    uint64_t max = std::numeric_limits<uint64_t>::max();
    uint64_t it = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (uint64_t i=max; i>(max-it); i--) {
        encode_ULEB128(i, buffer);
        uint64_t i2 = decode_ULEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_ULEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, UnsignedMinInt64) {
    uint64_t it = 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (uint64_t i=0; i<it; i++) {
        encode_ULEB128(i, buffer);
        uint64_t i2 = decode_ULEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_ULEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}

TEST(VariableSizeInteger, UnsignedMiddleInt64) {
    uint64_t max = std::numeric_limits<uint64_t>::max();
    uint64_t start = (max/2) - 10000;
    uint64_t end = (max/2) + 10000;
    size_t size;
    std::vector<uint8_t> buffer;
    for (uint64_t i=start; i<end; i++) {
        encode_ULEB128(i, buffer);
        uint64_t i2 = decode_ULEB128(buffer.data(), &size);
        ASSERT_EQ(buffer.size(), size) << "The encoded size and the decoded size are not equal";
        ASSERT_EQ(get_ULEB128_size(i), size) << "The encoded size does not match the predicted size";
        ASSERT_EQ(i, i2);
        buffer.clear();
    }
}
