#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

namespace route {

struct RoutingSettings {
    int bus_wait_time;
    double bus_velocity;
};

class TransportRouter {
   public:
    struct Edge {
        std::string_view from;
        std::string_view to;
        double weight;
        std::string_view bus_name;
        int span_count;
    };
    struct RouteInfo {
        double bus_wait_time;
        double total_weight;
        std::vector<TransportRouter::Edge> edges;
    };

    TransportRouter(const TransportCatalogue& catalogue, RoutingSettings&& routing_settings);

    std::optional<typename TransportRouter::RouteInfo> GetRouteInfo(const std::string& from,
                                                                    const std::string& to) const;

   private:
    mutable std::unordered_map<std::string, graph::VertexId> stop_to_vertex_;  // should be initialized here, before using
    const RoutingSettings routing_settings_;
    const graph::DirectedWeightedGraph<double> graph_;
    const graph::Router<double> router_;

    graph::DirectedWeightedGraph<double> GetCreatedGraph(
        const TransportCatalogue& catalogue, const RoutingSettings& routing_settings) const;

    std::optional<graph::VertexId> GetExistedVertexId(const std::string& stop_name) const;
    std::optional<graph::VertexId> GetOrCreateVertexId(const std::string& stop_name) const;

    std::string_view GetStopNameByVertexId(graph::VertexId vertex_id) const;
};

}  // namespace route
