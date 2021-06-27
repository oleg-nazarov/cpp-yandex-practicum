#include "stat_reader.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

#include "request_processing.h"
#include "transport_catalogue.h"

namespace route {
namespace request {

void HandleStat(const TransportCatalogue& catalogue) {
    std::string line;

    size_t requests_count;
    std::getline(std::cin, line);
    std::stringstream requests_count_ss(line);
    requests_count_ss >> requests_count;

    for (size_t _ = 0; _ < requests_count; ++_) {
        std::getline(std::cin, line);

        const Request request = GetProcessedStatRequest(line);

        if (request.type == RequestType::BUS) {
            GetBusInfoRequest(catalogue, request);
        } else if (request.type == RequestType::STOP) {
            GetBusesByStopRequest(catalogue, request);
        }
    }
}

void GetBusInfoRequest(const TransportCatalogue& catalogue, const Request& request) {
    std::optional<BusInfo> bus_info = catalogue.GetBusInfo(request.object_name);

    std::cout << std::setprecision(6) << detail::BUS_SV << ' ' << request.object_name << ':' << ' ';
    if (bus_info.has_value()) {
        std::cout << bus_info.value();
    } else {
        std::cout << detail::NOT_FOUND_SV;
    }
    std::cout << '\n';
}

void GetBusesByStopRequest(const TransportCatalogue& catalogue, const Request& request) {
    std::optional<std::set<std::string_view>> buses = catalogue.GetBusesByStop(request.object_name);

    std::cout << detail::STOP_SV << ' ' << request.object_name << ':' << ' ';
    if (buses.has_value()) {
        if (buses.value().size() == 0u) {
            std::cout << detail::NO_BUSES_SV;
        } else {
            std::cout << detail::BUSES_SV << ' ';
            for (std::string_view bus : buses.value()) {
                std::cout << bus << ' ';
            }
        }
    } else {
        std::cout << detail::NOT_FOUND_SV;
    }
    std::cout << '\n';
}

std::ostream& operator<<(std::ostream& os, const BusInfo& info) {
    using namespace detail;

    os << info.stops_count << ' ' << STOPS_ON_ROUTE_SV << ',' << ' ' << info.unique_stops_count << ' '
       << UNIQUE_STOPS_SV << ',' << ' ' << info.road_distance << ' ' << ROUTE_LENGTH_SV << ',' << ' '
       << info.curvature << ' ' << CURVATURE_SV;

    return os;
}

}  // namespace request
}  // namespace route
