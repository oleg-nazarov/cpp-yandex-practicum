#pragma once

#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "graph.h"
#include "map_renderer.h"
#include "router.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace route {

class RequestHandler {
   public:
    RequestHandler(const TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer,
                   const TransportRouter& transport_router);

    std::optional<BusInfo> GetBusInfo(std::string bus_name) const;

    std::optional<const std::set<std::string_view>*> GetBusesByStop(std::string_view stop_name) const;

    std::string GetMapSVG();

    std::optional<typename TransportRouter::RouteInfo> GetRouteInfo(const std::string& from,
                                                           const std::string& to) const;

   private:
    const TransportCatalogue& catalogue_;
    renderer::MapRenderer& map_renderer_;
    const TransportRouter& transport_router_;
};

}  // namespace route
