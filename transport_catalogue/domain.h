#pragma once

#include <set>
#include <string_view>
#include <vector>

#include "geo.h"

namespace route {

using DistanceType = unsigned long long;

struct BusInfo {
    BusInfo();
    BusInfo(size_t s_c, size_t u_s_c, double e_d, DistanceType r_d);

    size_t stops_count;
    size_t unique_stops_count;
    double euclidean_distance;
    DistanceType road_distance;
    double curvature;
};

struct Bus {
    // stop1, ...stopN-1, stopN, stopN-1, ...stop1
    std::vector<std::string_view> stops;
    std::string_view name;
    BusInfo info;
    bool is_roundtrip;
};

struct Stop {
    geo::Coordinates coordinates;
    std::set<std::string_view> buses;
};

}  // namespace route
