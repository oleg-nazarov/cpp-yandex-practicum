#include "input_reader.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "request_processing.h"
#include "transport_catalogue.h"

namespace route {
namespace request {

void HandleInput(TransportCatalogue& catalogue) {
    std::string line;

    std::getline(std::cin, line);
    int requests_count = std::stoi(line);

    for (size_t _ = 0; _ < requests_count; ++_) {
        std::getline(std::cin, line);
        const Request request = GetProcessedInputRequest(line);

        if (request.type == RequestType::STOP) {
            HandleAddStop(catalogue, request);
        } else if (request.type == RequestType::BUS) {
            HandleAddBus(catalogue, request);
        }
    }

    // stop info can appear after referencing it in a bus description, so we calculate and
    // store all buses' info after we handle all pack of requests
    catalogue.UpdateAllBusesInfo();
}

void HandleAddStop(TransportCatalogue& catalogue, const Request& request) {
    double latitude = std::stod(std::string(request.data[0]));
    double longitude = std::stod(std::string(request.data[1]));

    // if there is info about distances except latitude and longitude
    if (request.data.size() > 2u) {
        std::vector<std::pair<std::string_view, std::string_view>> distances_sv =
            GetProcessedDistances(request.data.begin() + 2, request.data.end());

        std::vector<Distance> distances;
        distances.reserve(distances_sv.size());

        std::transform(
            distances_sv.begin(), distances_sv.end(),
            std::back_inserter(distances),
            [&request](const std::pair<std::string_view, std::string_view>& d_sv) {
                unsigned long long distance = std::stoll(std::string(d_sv.second));

                Distance d_struct{request.object_name, d_sv.first, distance};

                return d_struct;
            });

        catalogue.AddStop(request.object_name, latitude, longitude, std::move(distances));
    } else {
        catalogue.AddStop(request.object_name, latitude, longitude);
    }
}

void HandleAddBus(TransportCatalogue& catalogue, const Request& request) {
    if (request.data_delimiter == detail::ROUND_TRIP_DELIMITER_SV) {
        catalogue.AddBusFromRoundTrip(request.object_name, request.data);
    } else if (request.data_delimiter == detail::ONE_WAY_DELIMITER_SV) {
        catalogue.AddBusFromOneWay(request.object_name, request.data);
    }
}

}  // namespace request
}  // namespace route
