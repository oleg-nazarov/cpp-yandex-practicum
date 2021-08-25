#pragma once

#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "map_renderer.h"
#include "transport_catalogue.h"

namespace route {

class RequestHandler {
   public:
    RequestHandler(const TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer);

    std::optional<BusInfo> GetBusInfo(std::string bus_name) const;

    std::optional<const std::set<std::string_view>*> GetBusesByStop(std::string_view stop_name) const;

    std::string GetMapSVG();

   private:
    const TransportCatalogue& catalogue_;
    renderer::MapRenderer& map_renderer_;
};

}  // namespace route
