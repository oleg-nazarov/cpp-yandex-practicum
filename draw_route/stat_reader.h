#pragma once

#include <iostream>

#include "transport_catalogue.h"

namespace route {
namespace stat_request {

void Read(std::istream& input, std::ostream& output, const TransportCatalogue& catalogue);

std::ostream& operator<<(std::ostream& os, const BusInfo& info);

namespace detail {

const std::string_view STOP_SV = "Stop";
const std::string_view BUS_SV = "Bus";

const std::string_view STOPS_ON_ROUTE_SV = "stops on route";
const std::string_view UNIQUE_STOPS_SV = "unique stops";
const std::string_view ROUTE_LENGTH_SV = "route length";
const std::string_view BUSES_SV = "buses";
const std::string_view NOT_FOUND_SV = "not found";
const std::string_view NO_BUSES_SV = "no buses";
const std::string_view CURVATURE_SV = "curvature";

enum class RequestType {
    GET_STOP,
    GET_BUS,
};

struct Request {
    Request(RequestType&& type, std::string&& object_name);

    RequestType type;
    std::string object_name;
};

Request GetProcessedRequest(std::string_view line_sv);

RequestType GetRequestType(std::string_view& line_sv);
std::string GetObjectName(std::string_view& line_sv);

void PrintBusInfo(std::ostream& output, const TransportCatalogue& catalogue, const Request& request);

void PrintStopToBuses(std::ostream& output, const TransportCatalogue& catalogue, const Request& request);

}  // namespace detail

}  // namespace stat_request
}  // namespace route
