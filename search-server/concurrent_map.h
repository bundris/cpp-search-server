#pragma once
#include <algorithm>
#include <map>
#include <numeric>
#include <string>
#include <vector>
#include <mutex>
#include <execution>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    explicit ConcurrentMap(size_t bucket_count)
            : maps_(bucket_count)
            , size_(bucket_count) {}

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    Access operator[](const Key& key) {
        size_t ind = static_cast<uint64_t>(key) % size_;
        return {std::lock_guard(maps_[ind].smtx), maps_[ind].smap[key]};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> temp;
        for (auto& m: maps_){
            {
                std::lock_guard<std::mutex> guard(m.smtx);
                temp.merge(m.smap);
            }
        }
        return temp;
    }

private:
    struct SubMap {
        std::map<Key, Value> smap;
        std::mutex smtx;
    };

    std::vector<SubMap> maps_;
    const size_t size_;
};