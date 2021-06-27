#include "request_processing.h"

#include <string>
#include <string_view>
#include <vector>

namespace route {
namespace request {

Request::Request(RequestType&& type, std::string_view object_name)
    : type(type), object_name(object_name) {}
Request::Request(RequestType&& type, std::string_view object_name,
                 std::vector<std::string_view>&& data, std::string_view data_delimiter)
    : type(type), object_name(object_name), data(data), data_delimiter(data_delimiter) {}

Request GetProcessedInputRequest(std::string_view line_sv) {
    RequestType request_type = GetRequestType(line_sv);
    std::string_view object_name = GetObjectName(line_sv);
    DataStruct data_struct = GetDataStruct(line_sv);

    Request request{std::move(request_type), object_name, std::move(data_struct.data), data_struct.delimiter};

    return request;
}

Request GetProcessedStatRequest(std::string_view line_sv) {
    RequestType request_type = GetRequestType(line_sv);
    std::string_view object_name = GetObjectName(line_sv);

    Request request{std::move(request_type), object_name};

    return request;
}

RequestType GetRequestType(std::string_view& line_sv) {
    size_t n_start = line_sv.find_first_not_of(' ');
    size_t n_end = line_sv.find(' ', n_start);

    std::string_view request_type_sv = line_sv.substr(n_start, n_end - n_start);
    line_sv.remove_prefix(n_end + 1);

    return request_type_sv == detail::STOP_SV ? RequestType::STOP : RequestType::BUS;
}

std::string_view GetObjectName(std::string_view& line_sv) {
    size_t n_start = line_sv.find_first_not_of(' ');
    size_t n_end = line_sv.find(':', n_start);

    std::string_view object_name = line_sv.substr(n_start, n_end - n_start);

    line_sv.remove_prefix(n_end + 1);

    return object_name;
}

DataStruct GetDataStruct(std::string_view& line_sv) {
    using namespace std::literals::string_literals;

    std::vector<std::string_view> data;

    size_t n_delimiter = line_sv.find_first_of(detail::ALL_DELIMITERS);
    std::string_view data_delimiter = line_sv.substr(n_delimiter, 1);

    while (true) {
        size_t n_start = line_sv.find_first_not_of(' ');
        size_t n_end = line_sv.find_first_of(data_delimiter, n_start);

        // value without delimiter
        std::string_view value = n_end == line_sv.npos ? line_sv.substr(n_start) : line_sv.substr(n_start, n_end - n_start);

        size_t n_remove_spaces = value.size() - 1u - value.find_last_not_of(' ');
        value.remove_suffix(n_remove_spaces);

        data.push_back(value);

        if (n_end == line_sv.npos) {
            break;
        } else {
            line_sv.remove_prefix(n_end + 1);
        }
    }

    DataStruct data_struct{data, data_delimiter};

    return data_struct;
}

std::vector<std::pair<std::string_view, std::string_view>> GetProcessedDistances(
    std::vector<std::string_view>::const_iterator begin_it,
    std::vector<std::string_view>::const_iterator end_it) {
    // first - stop_name, second - distance
    std::vector<std::pair<std::string_view, std::string_view>> distances_sv;

    for (auto it = begin_it; it != end_it; ++it) {
        std::string_view dist_description = *it;

        size_t n_start_distance = dist_description.find_first_not_of(' ');
        size_t n_end_distance = dist_description.find(detail::DISTANCE_VALUE_DELIMITER_SV);
        std::string_view distance = dist_description.substr(n_start_distance, n_end_distance - n_start_distance);

        size_t n_to = dist_description.find(detail::TO_SV, n_end_distance + 1);
        size_t n_start_stop = dist_description.find_first_not_of(' ', n_to + detail::TO_SV.size());

        std::string_view stop_name = dist_description.substr(n_start_stop);

        distances_sv.push_back(std::make_pair(stop_name, distance));
    }

    return distances_sv;
}

}  // namespace request
}  // namespace route
