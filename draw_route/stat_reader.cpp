#include "stat_reader.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "transport_catalogue.h"

namespace route {
namespace io {
namespace stat_request {

void __DEPRECATED__Read(std::istream& input, std::ostream& output, const TransportCatalogue& catalogue) {
    using namespace detail;

    std::string line;

    std::getline(input, line);
    int requests_count = std::stoi(line);

    for (int _ = 0; _ < requests_count; ++_) {
        std::getline(input, line);

        const Request request = detail::GetProcessedRequest(line);

        if (request.type == RequestType::GET_BUS) {
            PrintBusInfo(output, catalogue, request);
        } else if (request.type == RequestType::GET_STOP) {
            PrintStopToBuses(output, catalogue, request);
        }
    }
}

std::ostream& operator<<(std::ostream& os, const BusInfo& info) {
    using namespace detail;

    os << info.stops_count << ' ' << STOPS_ON_ROUTE_SV << ',' << ' ' << info.unique_stops_count << ' '
       << UNIQUE_STOPS_SV << ',' << ' ' << info.road_distance << ' ' << ROUTE_LENGTH_SV << ',' << ' '
       << info.curvature << ' ' << CURVATURE_SV;

    return os;
}

namespace detail {

Request::Request(RequestType&& type, std::string&& object_name) : type(std::move(type)),
                                                                  object_name(std::move(object_name)) {}

Request GetProcessedRequest(std::string_view line_sv) {
    RequestType request_type = GetRequestType(line_sv);
    std::string object_name = GetObjectName(line_sv);

    Request request{std::move(request_type), std::move(object_name)};

    return request;
}

RequestType GetRequestType(std::string_view& line_sv) {
    size_t n_start = line_sv.find_first_not_of(' ');
    size_t n_end = line_sv.find(' ', n_start);

    std::string_view request_type_sv = line_sv.substr(n_start, n_end - n_start);
    line_sv.remove_prefix(n_end + 1);

    return request_type_sv == STOP_SV ? RequestType::GET_STOP : RequestType::GET_BUS;
}

std::string GetObjectName(std::string_view& line_sv) {
    size_t n_start = line_sv.find_first_not_of(' ');
    size_t n_end = line_sv.find(':', n_start);

    std::string_view object_name_sv = line_sv.substr(n_start, n_end - n_start);

    line_sv.remove_prefix(n_end + 1);

    return std::string(object_name_sv);
}

void PrintBusInfo(std::ostream& output, const TransportCatalogue& catalogue, const Request& request) {
    std::optional<BusInfo> bus_info = catalogue.GetBusInfo(request.object_name);

    output << std::setprecision(6) << BUS_SV << ' ' << request.object_name << ':' << ' ';
    if (bus_info.has_value()) {
        output << bus_info.value();
    } else {
        output << NOT_FOUND_SV;
    }
    output << '\n';
}

void PrintStopToBuses(std::ostream& output, const TransportCatalogue& catalogue, const Request& request) {
    std::optional<std::set<std::string_view>> buses = catalogue.GetBusesByStop(request.object_name);

    output << STOP_SV << ' ' << request.object_name << ':' << ' ';
    if (buses.has_value()) {
        if (buses.value().size() == 0u) {
            output << NO_BUSES_SV;
        } else {
            output << BUSES_SV << ' ';

            for (auto bus_it = buses.value().begin(); bus_it != buses.value().end(); /* ++ inside */) {
                output << *bus_it;

                ++bus_it;
                if (bus_it != buses.value().end()) {
                    output << ' ';
                }
            }
        }
    } else {
        output << NOT_FOUND_SV;
    }
    output << '\n';
}

}  // namespace detail

}  // namespace stat_request
}  // namespace io
}  // namespace route
