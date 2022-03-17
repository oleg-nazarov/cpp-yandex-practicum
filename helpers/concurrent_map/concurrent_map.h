#pragma once

#include <cmath>
#include <iostream>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <vector>

template <typename Key, typename Value>
class ConcurrentMap {
   public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        Access(Value& value, std::mutex& m) : ref_to_value(value), m_(m) {}
        ~Access() {
            m_.unlock();
        }

        Value& ref_to_value;
        std::mutex& m_;
    };

    explicit ConcurrentMap(size_t bucket_count) : map_v_(std::vector<std::map<Key, Value>>(bucket_count)),
                                                  mutex_v_(std::vector<std::mutex>(bucket_count)){};

    Access operator[](const Key& key) {
        int idx = std::abs(static_cast<int>(key)) % static_cast<int>(map_v_.size());

        mutex_v_[idx].lock();

        return {map_v_[idx][key], mutex_v_[idx]};
    };

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> m;

        for (size_t i = 0; i < map_v_.size(); ++i) {
            std::lock_guard guard(mutex_v_[i]);
            m.merge(map_v_[i]);
        }

        return m;
    };

   private:
    std::vector<std::map<Key, Value>> map_v_;
    std::vector<std::mutex> mutex_v_;
};
