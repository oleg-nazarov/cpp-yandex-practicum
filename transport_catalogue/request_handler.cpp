#include "request_handler.h"

#include <iterator>
#include <string>

namespace route {

RequestHandler::RequestHandler(const TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer,
                               const TransportRouter& transport_router)
    : catalogue_(catalogue),
      map_renderer_(map_renderer),
      transport_router_(transport_router) {}

std::optional<BusInfo> RequestHandler::GetBusInfo(std::string bus_name) const {
    return catalogue_.GetBusInfo(std::move(bus_name));
}

std::optional<const std::set<std::string_view>*> RequestHandler::GetBusesByStop(std::string_view stop_name) const {
    return catalogue_.GetBusesByStop(stop_name);
}

std::string RequestHandler::GetMapSVG() {
    return map_renderer_.GetMapSVG(catalogue_.GetAllBuses(), catalogue_.GetAllStops());
}

std::optional<typename TransportRouter::RouteInfo> RequestHandler::GetRouteInfo(const std::string& from,
                                                                       const std::string& to) const {
    return transport_router_.GetRouteInfo(from, to);
}

}  // namespace route
