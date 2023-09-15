#ifndef OSTRICH_INTERVAL_LIST_H
#define OSTRICH_INTERVAL_LIST_H

#include <map>
#include <cstddef>
#include <cstring>
#include <limits>
#include "variable_size_integer.h"

/*
 * This class represent an ordered list of discrete elements supporting '==', '<', and '+' operators
 * The representation is compressed such that only intervals of values are stored.
 * Works preferably with trivial types which can be fully copied with memcpy() for serialization
 */
template<class T>
class IntervalList {
private:
    T max;
    std::map<T, T> intervals;

public:
    explicit IntervalList(T max_value): max(max_value) {}

    /**
     * Add new element to the list
     * @param element the new element
     */
    bool addition(T element) {
        bool has_changed = false;
        auto pos = intervals.lower_bound(element);
        if (pos == intervals.end()) {
            if (pos != intervals.begin()) {
                pos--;
                if (pos->first < element && pos->second == element) {
                    intervals[pos->first] = max;
                    has_changed = true;
                } else if (pos->second < element) {
                    intervals.insert(std::make_pair(element, max));
                    has_changed = true;
                }
            } else {
                intervals.insert(std::make_pair(element, max));
                has_changed = true;
            }
        } else {
            if (element < pos->first) {
                pos--;
                if (pos->second < element) {
                    pos++;
                    T old_del_val = pos->second;
                    intervals.erase(pos);
                    intervals.insert(std::make_pair(element, old_del_val));
                    has_changed = true;
                } else if (pos->second == element) {
                    auto tmp_pos = pos;
                    if (++tmp_pos != intervals.end()) {
                        T old_del_val = tmp_pos->second;
                        intervals.erase(tmp_pos);
                        pos->second = old_del_val;
                        has_changed = true;
                    }
                }
            }
        }
        return has_changed;
    }

    /**
     * Add the element as a single value
     * @param element
     * @return
     */
    bool lone_addition(T element) {
        bool hs = false;
        auto inter = get_interval(element);
        if (inter.first == max && inter.second == max) {  // the value don't exist in an existing interval
            intervals[element] = element+1;
            hs = true;
        }
        return hs;
    }

    /**
     * Delete element from the list (which possibly mark the end of an interval)
     * @param element the deleted element
     */
    bool deletion(T element) {
        if (intervals.empty()) {
            return false;
        }
        auto pos = intervals.lower_bound(element);
        if (pos == intervals.end() || element < pos->first) {
            pos--;
        }
        if (element < pos->second) {
            if (pos->first == element) {
                intervals.erase(pos);
            } else {
                pos->second = element;
            }
            return true;
        }
        return false;
    }
    /**
     * Tell if the element exist in the list
     * @param element
     * @return boolean on the existence of the element
     */
    [[nodiscard]] bool is_in(T element) const {
        auto pos = intervals.lower_bound(element);
        if (pos == intervals.end()) {
            if (pos == intervals.begin()) {
                return false;
            }
            pos--;
        } else if (element < pos->first) {
            pos--;
        }
        return pos->first <= element && element < pos->second;
    }
    /**
     * Return the interval in the internal representation that contain the value
     * @param element the value to search for
     * @return
     */
    std::pair<T,T> get_interval(T element) const {
        auto pos = intervals.lower_bound(element);
        if (pos == intervals.end()) {
            if (pos == intervals.begin()) {
                return std::make_pair(max,max);
            }
            pos--;
        }
        if (element < pos->first) pos--;
        if (pos->second < element) return std::make_pair(max,max);
        return std::make_pair(pos->first, pos->second);
    }

    T get_max_value() const {
        return max;
    }

    /**
     * Get the index of the value in the "virtual" value vector
     * @param value the value to look for
     * @param outer_limit the maximum value stored, instead of the infinite limit of open intervals
     * @return the index or -1 the value do not exist
     */
    long get_index(T value, T outer_limit) const {
        int count = 0;
        for (auto& inter: intervals) {
            int max_value = inter.second == max ? outer_limit : inter.second;
            int range = max_value - inter.first;
            if (inter.first <= value && value < max_value) {
                return count + (value - inter.first);
            }
            count += range;
        }
        return -1;
    }

    /**
     * Get the element at the given index in the "virtual" value vector
     * @param i the desired index
     * @param outer_limit the maximum value stored, instead of the infinite limit of open intervals
     * @return the element at the index or "max" if not found
     */
    T get_element_at(long i, T outer_limit) const {
        int index = 0;
        for (auto& inter: intervals) {
            int max_value = inter.second == max ? outer_limit : inter.second;
            int range = max_value - inter.first;
            if (i < index + range) {
                return inter.first + (i - index);
            }
            index += range;
        }
        return max;
    }

    /**
     * Get the size of the "virtual" value vector
     * @param outer_limit the maximum value stored, instead of the infinite limit of open intervals
     * @return the size
     */
    long get_size(T outer_limit) const {
        int size = 0;
        for (auto& inter: intervals) {
            int max_value = inter.second == max ? outer_limit : inter.second;
            int range = max_value - inter.first;
            size += range;
        }
        return size;
    }

    /**
     * Serialize the data into a byte stream
     * @return pair (data, size)
     */
    [[nodiscard]] std::pair<const char*, size_t> serialize() const {
#ifdef USE_VSI
        size_t unit_size_bytes = get_SLEB128_size(std::numeric_limits<T>::max());
#else
        size_t unit_size_bytes = sizeof(T);
#endif
        size_t alloc_size = unit_size_bytes * 2 * intervals.size();
        if (intervals.size() > 0) {
            char* data = new char[alloc_size];
            size_t offset = 0;
#ifdef USE_VSI
            std::vector<uint8_t> buffer;
#endif
            for (auto& t: intervals) {
#ifdef USE_VSI
                encode_SLEB128(t.first, buffer);
                std::memcpy(data+offset, buffer.data(), buffer.size());
                offset += buffer.size();
                buffer.clear();

                encode_SLEB128(t.second, buffer);
                std::memcpy(data+offset, buffer.data(), buffer.size());
                offset += buffer.size();
                buffer.clear();
#else
                std::memcpy(data+offset, &(t.first), sizeof(T));
                offset += sizeof(T);
                std::memcpy(data+offset, &(t.second), sizeof(T));
                offset += sizeof(T);
#endif
            }
            return std::make_pair(data, offset);
        }
        return std::make_pair(nullptr, 0);
    }

    /**
     * Load the data from a byte stream
     * @param data
     * @param size
     */
    void deserialize(const char *data, size_t size) {
        intervals.clear();
        size_t i = 0;
#ifdef USE_VSI
        size_t decode_size;
#endif
        while (i < size) {
            T s, e;
#ifdef USE_VSI
            s = decode_SLEB128((const uint8_t*)(data+i), &decode_size);
            i += decode_size;
            e = decode_SLEB128((const uint8_t*)(data+i), &decode_size);
            i += decode_size;
#else
            std::memcpy(&s, data+i, sizeof(T));
            i += sizeof(T);
            std::memcpy(&e, data+i, sizeof(T));
            i += sizeof(T);
#endif
            intervals.insert(std::make_pair(s, e));
        }
    }

    /**
     * Clear the data contained in the structure
     */
    void clear() {
        intervals.clear();
    }

    std::string to_string() const {
        std::string s;
        for (auto& kv: intervals) {
            s += "(" + std::to_string(kv.first) + "," + std::to_string(kv.second) + ") ";
        }
        return s;
    }

    void set_max_value(T value) {
        max = value;
    }

};

#endif //OSTRICH_INTERVAL_LIST_H
