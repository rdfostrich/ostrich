#ifndef OSTRICH_INTERVAL_LIST_H
#define OSTRICH_INTERVAL_LIST_H

#include <map>
#include <cstddef>
#include <cstring>

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

    /**
     * Give access to a const version of the interval data
     * @return the data
     */
    [[nodiscard]] const std::map<T, T>& get_internal_representation() const {
        return intervals;
    }

    T get_max_value() const {
        return max;
    }
    /**
     * Serialize the data into a byte stream
     * @return pair (data, size)
     */
    [[nodiscard]] std::pair<const char*, size_t> serialize() const {
        size_t size = sizeof(T) * 2 * intervals.size();
        if (size > 0) {
            char* data = new char[size];
            char* p = data;
            for (auto t: intervals) {
                std::memcpy(p, &(t.first), sizeof(T));
                p += sizeof(T);
                std::memcpy(p, &(t.second), sizeof(T));
                p += sizeof(T);
            }
            return std::make_pair(data, size);
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
        while (i < size) {
            T s, e;
            std::memcpy(&s, data+i, sizeof(T));
            i += sizeof(T);
            std::memcpy(&e, data+i, sizeof(T));
            i += sizeof(T);
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
        for (auto kv: intervals) {
            s += "(" + std::to_string(kv.first) + "," + std::to_string(kv.second) + ") ";
        }
        return s;
    }

};

#endif //OSTRICH_INTERVAL_LIST_H
