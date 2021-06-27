#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>
#include <execution>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geo.h"

namespace route {

BusInfo::BusInfo() = default;
BusInfo::BusInfo(size_t s_c, size_t u_s_c, double e_d, unsigned long long r_d)
    : stops_count(s_c), unique_stops_count(u_s_c), euclidean_distance(e_d), road_distance(r_d), curvature(static_cast<double>(r_d) / e_d) {}

void TransportCatalogue::AddStop(std::string_view name, double latitude, double longitude) {
    std::string_view stop_name_sv = GetOneOriginalNameSV(name, stop_names_, stops_);

    stops_[stop_name_sv].coordinates = {latitude, longitude};
}

void TransportCatalogue::AddStop(std::string_view name, double latitude, double longitude,
                                 std::vector<Distance>&& stop_distances) {
    AddStop(std::move(name), latitude, longitude);

    for (const Distance& distance : stop_distances) {
        // stops can appear at bus's description before being inserted into stops_
        std::string_view from_sv = GetOneOriginalNameSV(distance.from, stop_names_, stops_);
        std::string_view to_sv = GetOneOriginalNameSV(distance.to, stop_names_, stops_);

        stop_stop_distances_[from_sv][to_sv] = distance.distance;

        // besides writing "from-to" distance we also must write "to-from" one, if only we
        // already had not written a distance with "to-from" names, which on previous AddStop() call
        //  was "from-to" ("from-to" has a higher priority than "to-from")
        if (stop_stop_distances_.count(to_sv) == 0u ||
            stop_stop_distances_[to_sv].count(from_sv) == 0u) {
            stop_stop_distances_[to_sv][from_sv] = distance.distance;
        }
    }
}

void TransportCatalogue::AddBusFromOneWay(std::string_view name, const std::vector<std::string_view>& one_way_stops) {
    std::vector<std::string_view> stop_names = GetOriginalStopNamesSV(one_way_stops);
    std::vector<std::string_view> reversed_stop_names(stop_names.rbegin(), stop_names.rend());
    stop_names.insert(stop_names.end(), std::move_iterator(reversed_stop_names.begin() + 1),
                      std::move_iterator(reversed_stop_names.end()));

    AddBus(name, stop_names);
}

void TransportCatalogue::AddBusFromRoundTrip(std::string_view name, const std::vector<std::string_view>& round_trip_stops) {
    std::vector<std::string_view> stop_names = GetOriginalStopNamesSV(round_trip_stops);

    AddBus(name, stop_names);
}

StopPoint TransportCatalogue::GetStop(std::string_view name) const {
    return stops_.at(name);
}

void TransportCatalogue::UpdateAllBusesInfo() {
    for (auto it = buses_.begin(); it != buses_.end(); ++it) {
        UpdateBusInfo(it->first);
    }
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const {
    if (buses_.count(name) == 0u) {
        return std::nullopt;
    }

    return buses_.at(name).info;
}

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string_view>& stops) {
    std::string_view bus_name_sv = GetOneOriginalNameSV(name, bus_names_, buses_);

    for (std::string_view stop : stops) {
        stops_[stop].buses.insert(bus_name_sv);
    }

    buses_[bus_name_sv].stops = std::move(stops);
}

std::optional<std::set<std::string_view>> TransportCatalogue::GetBusesByStop(std::string_view stop) const {
    if (stops_.count(stop) == 0u) {
        return std::nullopt;
    }

    return stops_.at(stop).buses;
}

void TransportCatalogue::UpdateBusInfo(std::string_view name) {
    const std::vector<std::string_view> bus_stops = buses_.at(name).stops;

    double euclidean_distance = GetEuclideanDistance(bus_stops);
    unsigned long long road_distance = GetRoadDistance(bus_stops);

    BusInfo bus_info{bus_stops.size(), GetBusUniqueStopsCount(name), euclidean_distance, road_distance};

    buses_[name].info = bus_info;
}

double TransportCatalogue::GetEuclideanDistance(const std::vector<std::string_view>& bus_stops) const {
    double total_distance = .0;

    for (size_t i = 1; i < bus_stops.size(); ++i) {
        StopPoint stop_from = GetStop(bus_stops.at(i - 1));
        StopPoint stop_to = GetStop(bus_stops.at(i));

        total_distance += ComputeDistance(stop_from.coordinates, stop_to.coordinates);
    }

    return total_distance;
}

unsigned long long TransportCatalogue::GetRoadDistance(const std::vector<std::string_view>& bus_stops) const {
    unsigned long long total_distance = 0;

    for (size_t i = 1; i < bus_stops.size(); ++i) {
        total_distance += stop_stop_distances_.at(bus_stops[i - 1]).at(bus_stops[i]);
    }

    return total_distance;
}

std::vector<std::string_view> TransportCatalogue::GetOriginalStopNamesSV(const std::vector<std::string_view>& stops) {
    std::vector<std::string_view> original_stop_names;
    original_stop_names.reserve(stops.size());

    std::transform(
        stops.begin(), stops.end(),
        std::back_inserter(original_stop_names),
        [&](std::string_view stop_name) {
            std::string_view stop_name_sv = GetOneOriginalNameSV(std::move(stop_name), stop_names_, stops_);

            return stop_name_sv;
        });

    return original_stop_names;
}

// if we already have added a stop (or a bus) with such name, we return an existed sv from stops_
// (or buses_) to it, otherwise, we create a new original string and add it to stop_names_ (or bus_names_)
template <typename T>
std::string_view TransportCatalogue::GetOneOriginalNameSV(
    std::string_view name_sv, std::unordered_set<std::string>& original_names,
    std::unordered_map<std::string_view, T>& container) {
    std::string_view original_name_sv;

    auto it = container.find(name_sv);

    if (it != container.end()) {
        original_name_sv = it->first;

        return original_name_sv;
    }

    std::string new_original_name(name_sv);

    auto [inserted_name_it, _] = original_names.insert(std::move(new_original_name));
    original_name_sv = *inserted_name_it;

    return original_name_sv;
}

size_t TransportCatalogue::GetBusUniqueStopsCount(std::string_view name) const {
    auto bus_it = buses_.find(name);

    if (bus_it != buses_.end()) {
        std::unordered_set<std::string_view> unique_stops{bus_it->second.stops.begin(), bus_it->second.stops.end()};

        return unique_stops.size();
    } else {
        return 0u;
    }
}

}  // namespace route
