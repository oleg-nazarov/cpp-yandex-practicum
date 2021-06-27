#pragma once

#include "request_processing.h"
#include "transport_catalogue.h"

namespace route {
namespace request {

namespace detail {

const std::string_view STOPS_ON_ROUTE_SV = "stops on route";
const std::string_view UNIQUE_STOPS_SV = "unique stops";
const std::string_view ROUTE_LENGTH_SV = "route length";
const std::string_view BUSES_SV = "buses";
const std::string_view NOT_FOUND_SV = "not found";
const std::string_view NO_BUSES_SV = "no buses";
const std::string_view CURVATURE_SV = "curvature";

}  // namespace detail

void HandleStat(const TransportCatalogue& catalogue);

void GetBusInfoRequest(const TransportCatalogue& catalogue, const Request& request);

void GetBusesByStopRequest(const TransportCatalogue& catalogue, const Request& request);

std::ostream& operator<<(std::ostream& os, const BusInfo& info);

}  // namespace request
}  // namespace route
