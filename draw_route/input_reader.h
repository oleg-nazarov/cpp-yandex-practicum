#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "transport_catalogue.h"

namespace route {
namespace input_request {

void Read(std::istream& input, TransportCatalogue& catalogue);

namespace detail {

enum class RequestType {
    ADD_STOP,
    ADD_BUS,
};

struct Request {
    Request(RequestType&& type, std::string&& object_name,
            std::vector<std::string>&& data, std::string&& data_delimiter);

    RequestType type;
    std::string object_name;
    std::vector<std::string> data;
    std::string data_delimiter;
};

const std::string_view STOP_SV = "Stop";
const std::string_view ALL_DELIMITERS = ",>-";
const std::string_view ROUND_TRIP_DELIMITER_SV = ">";
const std::string_view ONE_WAY_DELIMITER_SV = "-";

const std::string_view DISTANCE_VALUE_DELIMITER_SV = "m";
const std::string_view TO_SV = "to";

Request GetProcessedRequest(std::string_view line_sv);

RequestType GetRequestType(std::string_view& line_sv);
std::string GetObjectName(std::string_view& line_sv);
std::string GetDataDelimiter(std::string_view line_sv);
std::vector<std::string> GetData(std::string_view& line_sv, const std::string& delimiter);

std::vector<std::pair<std::string_view, std::string_view>> GetProcessedDistances(
    std::vector<std::string>::const_iterator begin_it,
    std::vector<std::string>::const_iterator end_it);

void HandleAddStop(TransportCatalogue& catalogue, const Request& request);

void HandleAddBus(TransportCatalogue& catalogue, const Request& request);

}  // namespace detail

}  // namespace input_request
}  // namespace route
