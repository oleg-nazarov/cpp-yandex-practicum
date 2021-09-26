#include "transport_router.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <string_view>
#include <vector>

#include "domain.h"

namespace route {

TransportRouter::TransportRouter(const TransportCatalogue& catalogue, RoutingSettings&& routing_settings)
    : routing_settings_(std::move(routing_settings)),
      graph_(GetCreatedGraph(catalogue, routing_settings)),
      router_(graph_) {}

std::optional<typename TransportRouter::RouteInfo> TransportRouter::GetRouteInfo(const std::string& from,
                                                                                 const std::string& to) const {
    std::optional<graph::VertexId> vertex_from = GetExistedVertexId(from);
    std::optional<graph::VertexId> vertex_to = GetExistedVertexId(to);
    if (!(vertex_from.has_value() && vertex_to.has_value())) {
        return std::nullopt;
    }

    std::optional<typename graph::Router<double>::RouteInfo> route_info =
        router_.BuildRoute(*vertex_from, *vertex_to);

    if (!route_info.has_value()) {
        return std::nullopt;
    }

    // replace edge_id by a reference to edge
    TransportRouter::RouteInfo transport_info;

    transport_info.bus_wait_time = routing_settings_.bus_wait_time;
    transport_info.total_weight = route_info->weight;
    transport_info.edges.reserve(route_info->edges.size());

    std::transform(
        route_info->edges.begin(), route_info->edges.end(),
        std::back_inserter(transport_info.edges),
        [&](graph::EdgeId edge_id) {
            const graph::Edge<double>& edge = graph_.GetEdge(edge_id);

            TransportRouter::Edge transport_edge{
                GetStopNameByVertexId(edge.from),
                GetStopNameByVertexId(edge.to),
                edge.weight,
                edge.name,
                edge.span_count};

            return transport_edge;
        });

    return transport_info;
}

std::string_view TransportRouter::GetStopNameByVertexId(graph::VertexId vertex_id) const {
    auto it = std::find_if(
        stop_to_vertex_.begin(), stop_to_vertex_.end(),
        [vertex_id](const auto& p) {
            return vertex_id == p.second;
        });

    return it->first;
}

graph::DirectedWeightedGraph<double> TransportRouter::GetCreatedGraph(
    const TransportCatalogue& catalogue, const RoutingSettings& routing_settings) const {
    graph::DirectedWeightedGraph<double> created_graph;

    const std::vector<const Bus*> all_buses = catalogue.GetAllBuses();

    for (const Bus* bus : all_buses) {
        for (size_t i = 0; i < bus->stops.size(); ++i) {
            std::string_view from = bus->stops[i];

            double edge_weight = routing_settings.bus_wait_time;

            for (size_t j = i + 1; j < bus->stops.size(); ++j) {
                std::string_view before_to = bus->stops[j - 1];
                std::string_view to = bus->stops[j];

                std::optional<DistanceType> distance = catalogue.GetDistanceBetweenStops(before_to, to);
                if (!distance.has_value()) {
                    continue;
                }

                // count of minutes
                edge_weight += (static_cast<double>(*distance) * 60.) / (routing_settings.bus_velocity * 1000.);

                int span_count = j - i;
                created_graph.AddEdge({GetOrCreateVertexId(std::string(from)).value(),
                                       GetOrCreateVertexId(std::string(to)).value(),
                                       edge_weight, bus->name, span_count});
            }
        }
    }

    return created_graph;
}

std::optional<graph::VertexId> TransportRouter::GetExistedVertexId(const std::string& stop_name) const {
    if (stop_to_vertex_.count(stop_name) == 0) {
        return std::nullopt;
    }

    return stop_to_vertex_.at(stop_name);
}

std::optional<graph::VertexId> TransportRouter::GetOrCreateVertexId(const std::string& stop_name) const {
    if (stop_to_vertex_.count(stop_name)) {
        return stop_to_vertex_.at(stop_name);
    }

    stop_to_vertex_[stop_name] = stop_to_vertex_.size();

    return GetExistedVertexId(stop_name);
}

}  // namespace route
