#include "input_reader.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <queue>
#include <string>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace route {
namespace io {
namespace input_request {

void __DEPRECATED__Read(std::istream& input, TransportCatalogue& catalogue) {
    using namespace detail;

    std::string line;

    std::getline(input, line);
    int requests_count = std::stoi(line);

    // stop info can appear after necessity to use it in a bus description, so firstly we handle
    // only STOP-requests and delay BUS-requests
    std::queue<Request> delayed_requests;

    for (int _ = 0; _ < requests_count; ++_) {
        std::getline(input, line);

        const Request request = GetProcessedRequest(line);

        if (request.type == RequestType::ADD_STOP) {
            HandleAddStop(catalogue, request);
        } else if (request.type == RequestType::ADD_BUS) {
            delayed_requests.push(std::move(request));
        }
    }

    while (!delayed_requests.empty()) {
        HandleAddBus(catalogue, delayed_requests.front());

        delayed_requests.pop();
    }
}

namespace detail {

Request::Request(RequestType&& type, std::string&& object_name, std::vector<std::string>&& data, std::string&& data_delimiter)
    : type(std::move(type)), object_name(std::move(object_name)), data(std::move(data)), data_delimiter(std::move(data_delimiter)) {}

Request GetProcessedRequest(std::string_view line_sv) {
    RequestType request_type = GetRequestType(line_sv);
    std::string object_name = GetObjectName(line_sv);
    std::string data_delimiter = GetDataDelimiter(line_sv);
    std::vector<std::string> data = GetData(line_sv, data_delimiter);

    Request request{std::move(request_type), std::move(object_name), std::move(data),
                    std::move(data_delimiter)};

    return request;
}

RequestType GetRequestType(std::string_view& line_sv) {
    size_t n_start = line_sv.find_first_not_of(' ');
    size_t n_end = line_sv.find(' ', n_start);

    std::string_view request_type_sv = line_sv.substr(n_start, n_end - n_start);
    line_sv.remove_prefix(n_end + 1);

    return request_type_sv == STOP_SV ? RequestType::ADD_STOP : RequestType::ADD_BUS;
}

std::string GetObjectName(std::string_view& line_sv) {
    size_t n_start = line_sv.find_first_not_of(' ');
    size_t n_end = line_sv.find(':', n_start);

    std::string_view object_name = line_sv.substr(n_start, n_end - n_start);

    line_sv.remove_prefix(n_end + 1);

    return std::string(object_name);
}

std::string GetDataDelimiter(std::string_view line_sv) {
    size_t n_delimiter = line_sv.find_first_of(ALL_DELIMITERS);
    std::string_view delimiter_sv = line_sv.substr(n_delimiter, 1);

    return std::string(delimiter_sv);
}

std::vector<std::string> GetData(std::string_view& line_sv, const std::string& delimiter) {
    using namespace std::literals::string_literals;

    std::vector<std::string> data;

    while (true) {
        size_t n_start = line_sv.find_first_not_of(' ');
        size_t n_end = line_sv.find_first_of(delimiter, n_start);

        // value without delimiter
        std::string_view value = n_end == line_sv.npos ? line_sv.substr(n_start) : line_sv.substr(n_start, n_end - n_start);

        size_t n_remove_spaces = value.size() - 1u - value.find_last_not_of(' ');
        value.remove_suffix(n_remove_spaces);

        data.push_back(std::string(value));

        if (n_end == line_sv.npos) {
            break;
        } else {
            line_sv.remove_prefix(n_end + 1);
        }
    }

    return data;
}

std::vector<std::pair<std::string_view, std::string_view>> GetProcessedDistances(
    std::vector<std::string>::const_iterator begin_it,
    std::vector<std::string>::const_iterator end_it) {
    // first - stop_name, second - distance
    std::vector<std::pair<std::string_view, std::string_view>> distances_sv;

    for (auto it = begin_it; it != end_it; ++it) {
        std::string_view dist_description = *it;

        size_t n_start_dist = dist_description.find_first_not_of(' ');
        size_t n_end_dist = dist_description.find(DISTANCE_VALUE_DELIMITER_SV);
        std::string_view distance = dist_description.substr(n_start_dist, n_end_dist - n_start_dist);

        size_t n_to = dist_description.find(TO_SV, n_end_dist + 1);
        size_t n_start_stop = dist_description.find_first_not_of(' ', n_to + TO_SV.size());

        std::string_view stop_name = dist_description.substr(n_start_stop);

        distances_sv.push_back(std::make_pair(stop_name, distance));
    }

    return distances_sv;
}

void HandleAddStop(TransportCatalogue& catalogue, const Request& request) {
    geo::Coordinates coordinates{std::stod(request.data[0]), std::stod(request.data[1])};

    catalogue.AddStop(request.object_name, coordinates);

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
                DistanceType distance = std::stoll(std::string(d_sv.second));

                Distance d{request.object_name, d_sv.first, distance};

                return d;
            });

        catalogue.SetDistances(std::move(distances));
    }
}

void HandleAddBus(TransportCatalogue& catalogue, const Request& request) {
    catalogue.AddBus(request.object_name, request.data, request.data_delimiter == ONE_WAY_DELIMITER_SV);
}

}  // namespace detail

}  // namespace input_request
}  // namespace io
}  // namespace route
