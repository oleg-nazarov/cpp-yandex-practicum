#pragma once

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "domain.h"
#include "geo.h"

namespace route {

struct Distance {
    std::string_view from;
    std::string_view to;
    DistanceType distance;
};

class TransportCatalogue {
   public:
    void AddStop(std::string_view name, geo::Coordinates& coordinates);

    void AddBus(std::string_view name, const std::vector<std::string>& stops, bool is_one_way_stops);

    Bus GetBus(std::string_view name) const;
    Stop GetStop(std::string_view name) const;

    std::vector<const Bus*> GetAllBuses() const;
    const std::unordered_map<std::string_view, Stop>& GetAllStops() const;

    std::optional<const std::set<std::string_view>*> GetBusesByStop(std::string_view stop) const;

    std::optional<BusInfo> GetBusInfo(std::string_view name) const;

    std::optional<DistanceType> GetDistanceBetweenStops(std::string_view from, std::string_view to) const;

    void SetDistances(std::vector<Distance>&& stop_distances);

   private:
    std::unordered_set<std::string> stop_names_;
    std::unordered_set<std::string> bus_names_;
    std::unordered_map<std::string_view, Stop> stops_;
    std::unordered_map<std::string_view, Bus> buses_;
    std::unordered_map<std::string_view, std::unordered_map<std::string_view, DistanceType>> stop_stop_distances_;

    BusInfo CalculateBusInfo(std::string_view name);

    double GetEuclideanDistance(const std::vector<std::string_view>& bus_stops) const;
    DistanceType GetRoadDistance(const std::vector<std::string_view>& bus_stops) const;

    template <typename T>
    std::string_view GetOneOriginalNameSV(std::string_view name_sv,
                                          std::unordered_set<std::string>& original_names,
                                          std::unordered_map<std::string_view, T>& container);

    std::vector<std::string_view> GetOriginalStopNamesSV(const std::vector<std::string>& stops);

    size_t GetBusUniqueStopsCount(std::string_view name) const;
};

}  // namespace route
