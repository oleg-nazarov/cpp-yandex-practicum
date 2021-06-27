#pragma once

#include <string_view>
#include <vector>

namespace route {
namespace request {

namespace detail {

const std::string_view STOP_SV = "Stop";
const std::string_view BUS_SV = "Bus";

const std::string_view ALL_DELIMITERS = ",>-";
const std::string_view ROUND_TRIP_DELIMITER_SV = ">";
const std::string_view ONE_WAY_DELIMITER_SV = "-";

const std::string_view DISTANCE_VALUE_DELIMITER_SV = "m";
const std::string_view TO_SV = "to";

}  // namespace detail

struct DataStruct {
    std::vector<std::string_view> data;
    std::string_view delimiter;
};

enum class RequestType {
    STOP,
    BUS,
};

struct Request {
    Request(RequestType&& type, std::string_view object_name);
    Request(RequestType&& type, std::string_view object_name,
            std::vector<std::string_view>&& data, std::string_view data_delimiter);

    RequestType type;
    std::string_view object_name;
    std::vector<std::string_view> data;
    std::string_view data_delimiter;
};

Request GetProcessedInputRequest(std::string_view line_sv);
Request GetProcessedStatRequest(std::string_view line_sv);

RequestType GetRequestType(std::string_view& line_sv);
std::string_view GetObjectName(std::string_view& line_sv);
DataStruct GetDataStruct(std::string_view& line_sv);

std::vector<std::pair<std::string_view, std::string_view>> GetProcessedDistances(
    std::vector<std::string_view>::const_iterator begin_it,
    std::vector<std::string_view>::const_iterator end_it);

}  // namespace request
}  // namespace route
