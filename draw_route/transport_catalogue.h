#pragma once

#include <cassert>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geo.h"

namespace route {

struct BusInfo {
    BusInfo();
    BusInfo(size_t s_c, size_t u_s_c, double e_d, unsigned long long r_d);

    size_t stops_count;
    size_t unique_stops_count;
    double euclidean_distance;
    unsigned long long road_distance;
    double curvature;
};

struct Bus {
    // stop1, ...stopN-1, stopN, stopN-1, ...stop1
    std::vector<std::string_view> stops;
    BusInfo info;
};

struct Distance {
    std::string_view from;
    std::string_view to;
    unsigned long long distance;
};

struct StopPoint {
    geo::Coordinates coordinates;
    std::set<std::string_view> buses;
};

class TransportCatalogue {
   public:
    void AddStop(std::string_view name, double latitude, double longitude);
    void AddStop(std::string_view name, double latitude, double longitude,
                 std::vector<Distance>&& stop_distances);

    void AddBusFromOneWay(std::string_view name, const std::vector<std::string_view>& one_way_stops);
    void AddBusFromRoundTrip(std::string_view name, const std::vector<std::string_view>& round_trip_stops);

    StopPoint GetStop(std::string_view name) const;

    std::optional<std::set<std::string_view>> GetBusesByStop(std::string_view stop) const;

    void UpdateAllBusesInfo();

    std::optional<BusInfo> GetBusInfo(std::string_view name) const;

   private:
    std::unordered_set<std::string> stop_names_;
    std::unordered_set<std::string> bus_names_;
    std::unordered_map<std::string_view, StopPoint> stops_;
    std::unordered_map<std::string_view, Bus> buses_;
    std::unordered_map<std::string_view, std::unordered_map<std::string_view, unsigned long long>> stop_stop_distances_;

    void AddBus(std::string_view name, const std::vector<std::string_view>& stops);

    void UpdateBusInfo(std::string_view name);

    double GetEuclideanDistance(const std::vector<std::string_view>& bus_stops) const;
    unsigned long long GetRoadDistance(const std::vector<std::string_view>& bus_stops) const;

    template <typename T>
    std::string_view GetOneOriginalNameSV(std::string_view name_sv,
                                          std::unordered_set<std::string>& original_names,
                                          std::unordered_map<std::string_view, T>& container);

    std::vector<std::string_view> GetOriginalStopNamesSV(const std::vector<std::string_view>& stops);

    size_t GetBusUniqueStopsCount(std::string_view name) const;
};

}  // namespace route
