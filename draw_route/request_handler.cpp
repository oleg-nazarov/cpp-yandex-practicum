#include "request_handler.h"

#include <iterator>

namespace route {

RequestHandler::RequestHandler(const TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer)
    : catalogue_(catalogue), map_renderer_(map_renderer) {}

std::optional<BusInfo> RequestHandler::GetBusInfo(std::string bus_name) const {
    return catalogue_.GetBusInfo(std::move(bus_name));
}

std::optional<const std::set<std::string_view>*> RequestHandler::GetBusesByStop(std::string_view stop_name) const {
    return catalogue_.GetBusesByStop(stop_name);
}

std::string RequestHandler::GetMapSVG() {
    return map_renderer_.GetMapSVG(catalogue_.GetAllBuses(), catalogue_.GetAllStops());
}

}  // namespace route
