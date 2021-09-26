#include "transport_catalogue.h"

#include <algorithm>
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

void TransportCatalogue::AddStop(std::string_view name, geo::Coordinates& coordinates) {
    std::string_view stop_name_sv = GetOneOriginalNameSV(name, stop_names_, stops_);

    stops_[stop_name_sv].coordinates = {coordinates.lat, coordinates.lng};
}

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string>& stops,
                                bool is_one_way_stops) {
    std::vector<std::string_view> stop_names_sv = GetOriginalStopNamesSV(stops);

    if (is_one_way_stops) {
        std::vector<std::string_view> reversed_stop_names_sv(stop_names_sv.rbegin(), stop_names_sv.rend());

        stop_names_sv.insert(stop_names_sv.end(), std::move_iterator(reversed_stop_names_sv.begin() + 1),
                             std::move_iterator(reversed_stop_names_sv.end()));
    }

    std::string_view bus_name_sv = GetOneOriginalNameSV(name, bus_names_, buses_);

    for (std::string_view stop_sv : stop_names_sv) {
        stops_[stop_sv].buses.insert(bus_name_sv);
    }

    buses_[bus_name_sv].stops = std::move(stop_names_sv);      // #1
    buses_[bus_name_sv].info = CalculateBusInfo(bus_name_sv);  // #2 (uses a consequence of #1)
    buses_[bus_name_sv].name = bus_name_sv;                    // TODO: may be change structure for storing only one bus_name
    buses_[bus_name_sv].is_roundtrip = !is_one_way_stops;
}

Bus TransportCatalogue::GetBus(std::string_view name) const {
    return buses_.at(name);
}

Stop TransportCatalogue::GetStop(std::string_view name) const {
    return stops_.at(name);
}

std::vector<const Bus*> TransportCatalogue::GetAllBuses() const {
    std::vector<const Bus*> buses(buses_.size());

    std::transform(
        std::execution::par,
        buses_.begin(), buses_.end(),
        buses.begin(),
        [](const auto& p) {
            return &(p.second);
        });

    return buses;
}

const std::unordered_map<std::string_view, Stop>& TransportCatalogue::GetAllStops() const {
    return stops_;
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const {
    if (buses_.count(name) == 0u) {
        return std::nullopt;
    }

    return GetBus(name).info;
}

std::optional<DistanceType> TransportCatalogue::GetDistanceBetweenStops(std::string_view from, std::string_view to) const {
    using namespace std::literals::string_literals;
    if (stop_stop_distances_.count(from) == 0 || stop_stop_distances_.at(from).count(to) == 0) {
        return std::nullopt;
    }

    return stop_stop_distances_.at(from).at(to);
}

std::optional<const std::set<std::string_view>*> TransportCatalogue::GetBusesByStop(std::string_view stop) const {
    if (stops_.count(stop) == 0u) {
        return std::nullopt;
    }

    return &(stops_.at(stop).buses);
}

void TransportCatalogue::SetDistances(std::vector<Distance>&& stop_distances) {
    for (const Distance& distance : stop_distances) {
        // stops can appear at bus's description before being inserted into stops_
        std::string_view from_sv = GetOneOriginalNameSV(distance.from, stop_names_, stops_);
        std::string_view to_sv = GetOneOriginalNameSV(distance.to, stop_names_, stops_);

        stop_stop_distances_[from_sv][to_sv] = distance.distance;

        // besides writing a "from-to" distance we also must write a "to-from" one, if we only had not
        // written a distance with "to-from" names at one of the previous "AddStopAndDistances()"-call,
        // which previously was "from-to" ("from-to" has a higher priority than "to-from")
        if (stop_stop_distances_.count(to_sv) == 0u ||
            stop_stop_distances_[to_sv].count(from_sv) == 0u) {
            stop_stop_distances_[to_sv][from_sv] = distance.distance;
        }
    }
}

BusInfo TransportCatalogue::CalculateBusInfo(std::string_view name) {
    const std::vector<std::string_view> bus_stops = buses_.at(name).stops;

    double euclidean_distance = GetEuclideanDistance(bus_stops);
    DistanceType road_distance = GetRoadDistance(bus_stops);

    BusInfo bus_info{bus_stops.size(), GetBusUniqueStopsCount(name), euclidean_distance, road_distance};

    return bus_info;
}

double TransportCatalogue::GetEuclideanDistance(const std::vector<std::string_view>& bus_stops) const {
    double total_distance = .0;

    for (size_t i = 1; i < bus_stops.size(); ++i) {
        Stop stop_from = GetStop(bus_stops.at(i - 1));
        Stop stop_to = GetStop(bus_stops.at(i));

        total_distance += ComputeDistance(stop_from.coordinates, stop_to.coordinates);
    }

    return total_distance;
}

DistanceType TransportCatalogue::GetRoadDistance(const std::vector<std::string_view>& bus_stops) const {
    DistanceType total_distance = 0;

    for (size_t i = 1; i < bus_stops.size(); ++i) {
        total_distance += stop_stop_distances_.at(bus_stops[i - 1]).at(bus_stops[i]);
    }

    return total_distance;
}

std::vector<std::string_view> TransportCatalogue::GetOriginalStopNamesSV(const std::vector<std::string>& stops) {
    std::vector<std::string_view> original_stop_names;
    original_stop_names.reserve(stops.size());

    std::transform(
        stops.begin(), stops.end(),
        std::back_inserter(original_stop_names),
        [&](const std::string& stop_name) {
            std::string_view stop_name_sv = GetOneOriginalNameSV(stop_name, stop_names_, stops_);

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
