#include "domain.h"

namespace route {

BusInfo::BusInfo() = default;
BusInfo::BusInfo(size_t s_c, size_t u_s_c, double e_d, DistanceType r_d)
    : stops_count(s_c), unique_stops_count(u_s_c), euclidean_distance(e_d), road_distance(r_d), curvature(static_cast<double>(r_d) / e_d) {}

}  // namespace route
