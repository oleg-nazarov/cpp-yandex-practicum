#include "serialization.h"

#include <transport_catalogue.pb.h>

#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "domain.h"
#include "svg/svg.h"
#include "transport_catalogue.h"

namespace route {
namespace io {

namespace detail {

struct NameToId {
    const std::unordered_map<std::string_view, uint32_t> stops;
    const std::unordered_map<std::string_view, uint32_t> buses;
};

// return helper that is used to store next references to the names as ids
const NameToId SerializeStopAndBusNames(route::serialize::TransportCatalogue& proto_catalogue,
                                        const TransportCatalogue& catalogue) {
    std::unordered_map<std::string_view, uint32_t> stop_name_to_id;
    std::unordered_map<std::string_view, uint32_t> bus_name_to_id;

    // serialize stop names
    auto& proto_id_to_stop_name = *proto_catalogue.mutable_id_to_stop_name();
    for (const auto& [stop_name_sv, _] : catalogue.GetAllStops()) {
        uint32_t id = proto_id_to_stop_name.size();

        proto_id_to_stop_name[id] = std::string(stop_name_sv);
        stop_name_to_id[stop_name_sv] = id;
    }

    // serialize bus names
    auto& proto_id_to_bus_name = *proto_catalogue.mutable_id_to_bus_name();
    for (const Bus* bus : catalogue.GetAllBuses()) {
        uint32_t id = proto_id_to_bus_name.size();

        proto_id_to_bus_name[id] = std::string(bus->name);
        bus_name_to_id[bus->name] = id;
    }

    const NameToId name_to_id{std::move(stop_name_to_id), std::move(bus_name_to_id)};
    return name_to_id;
}

route::serialize::Stop GetProtoStop(std::string_view stop_name_sv, const Stop& stop, const NameToId& name_to_id) {
    route::serialize::Stop proto_stop;

    uint32_t id = name_to_id.stops.at(stop_name_sv);
    proto_stop.set_stop_name_id(id);

    proto_stop.mutable_coordinates()->set_latitude(stop.coordinates.lat);
    proto_stop.mutable_coordinates()->set_longitude(stop.coordinates.lng);

    for (std::string_view bus_sv : stop.buses) {
        proto_stop.add_buses(name_to_id.buses.at(bus_sv));
    }

    return proto_stop;
}

route::serialize::Bus GetProtoBus(const Bus& bus, const NameToId& name_to_id) {
    route::serialize::Bus proto_bus;

    uint32_t id = name_to_id.buses.at(bus.name);
    proto_bus.set_bus_name_id(id);

    // we store stops in transport catalogue in a way that represents a roundtrip way
    // (no matter what "is_roundtrip"-flag actually is);
    // so if the flag is "false", we should serialize only half+1 of the stops (one way stops)
    auto it_end = bus.is_roundtrip ? bus.stops.end() : bus.stops.begin() + (bus.stops.size() / 2) + 1;
    for (auto it = bus.stops.begin(); it != it_end; ++it) {
        proto_bus.add_stops(name_to_id.stops.at(*it));
    }

    proto_bus.set_is_roundtrip(bus.is_roundtrip);

    return proto_bus;
}

route::serialize::StopToStopDistance GetProtoStopToStopDistance(std::string_view from_sv, std::string_view to_sv,
                                                                DistanceType distance, const NameToId& name_to_id) {
    route::serialize::StopToStopDistance proto_stop_to_stop_dist;

    uint32_t from_id = name_to_id.stops.at(from_sv);
    uint32_t to_id = name_to_id.stops.at(to_sv);

    proto_stop_to_stop_dist.set_from_id(from_id);
    proto_stop_to_stop_dist.set_to_id(to_id);
    proto_stop_to_stop_dist.set_distance(distance);

    return proto_stop_to_stop_dist;
}

// map settings

route::serialize::Color GetProtoColor(const svg::Color& color) {
    route::serialize::Color proto_color;

    if (std::holds_alternative<std::string>(color)) {
        proto_color.set_color_string(std::get<std::string>(color));
    } else if (std::holds_alternative<svg::Rgb>(color)) {
        svg::Rgb rgb_color = std::get<svg::Rgb>(color);

        route::serialize::ColorArray color_array;
        color_array.add_rgb(rgb_color.red);
        color_array.add_rgb(rgb_color.green);
        color_array.add_rgb(rgb_color.blue);

        *proto_color.mutable_color_array() = std::move(color_array);
    } else if (std::holds_alternative<svg::Rgba>(color)) {
        svg::Rgba rgba_color = std::get<svg::Rgba>(color);

        route::serialize::ColorArray color_array;
        color_array.add_rgb(rgba_color.red);
        color_array.add_rgb(rgba_color.green);
        color_array.add_rgb(rgba_color.blue);
        color_array.set_opacity(rgba_color.opacity);

        *proto_color.mutable_color_array() = std::move(color_array);
    }

    return proto_color;
}

svg::Color GetSvgColor(const route::serialize::Color& proto_color) {
    if (proto_color.has_color_string()) {
        svg::Color color{proto_color.color_string()};
        return color;
    } else if (proto_color.has_color_array()) {
        const auto& color_array = proto_color.color_array();

        if (color_array.has_opacity()) {
            svg::Rgba rgba{
                static_cast<uint8_t>(color_array.rgb()[0]),
                static_cast<uint8_t>(color_array.rgb()[1]),
                static_cast<uint8_t>(color_array.rgb()[2]),
                color_array.opacity()};
            return {rgba};
        } else {
            svg::Rgb rgb{
                static_cast<uint8_t>(color_array.rgb()[0]),
                static_cast<uint8_t>(color_array.rgb()[1]),
                static_cast<uint8_t>(color_array.rgb()[2])};
            return {rgb};
        }
    }

    return {};
}

route::serialize::RenderSettings GetProtoMapSettings(const renderer::MapSettings& map_settings) {
    route::serialize::RenderSettings proto_renderer_settings;

    proto_renderer_settings.set_width(map_settings.GetWidth());
    proto_renderer_settings.set_height(map_settings.GetHeight());
    proto_renderer_settings.set_padding(map_settings.GetPadding());
    proto_renderer_settings.set_stop_radius(map_settings.GetStopRadius());
    proto_renderer_settings.set_line_width(map_settings.GetLineWidth());

    proto_renderer_settings.set_bus_label_font_size(map_settings.GetBusLabelFontSize());
    for (double bus_label_offset : map_settings.GetBusLabelOffset()) {
        proto_renderer_settings.add_bus_label_offset(bus_label_offset);
    }

    proto_renderer_settings.set_stop_label_font_size(map_settings.GetStopLabelFontSize());
    for (double stop_label_offset : map_settings.GetStopLabelOffset()) {
        proto_renderer_settings.add_stop_label_offset(stop_label_offset);
    }

    svg::Color underlayer_color = map_settings.GetUnderlayerColor();
    *proto_renderer_settings.mutable_underlayer_color() = GetProtoColor(underlayer_color);

    proto_renderer_settings.set_underlayer_width(map_settings.GetUnderlayerWidth());

    for (const svg::Color& color_palette : map_settings.GetColorPalette()) {
        *proto_renderer_settings.add_color_palette() = GetProtoColor(color_palette);
    }

    return proto_renderer_settings;
}

void DeserializeMapSettings(renderer::MapSettings& renderer_settings, const route::serialize::RenderSettings& deserialized_settings) {
    renderer_settings.SetWidth(deserialized_settings.width());
    renderer_settings.SetHeight(deserialized_settings.height());
    renderer_settings.SetPadding(deserialized_settings.padding());
    renderer_settings.SetStopRadius(deserialized_settings.stop_radius());
    renderer_settings.SetLineWidth(deserialized_settings.line_width());
    renderer_settings.SetBusLabelFontSize(deserialized_settings.bus_label_font_size());

    std::vector<double> bus_label_offsets;
    for (double offset : deserialized_settings.bus_label_offset()) {
        bus_label_offsets.push_back(offset);
    }
    renderer_settings.SetBusLabelOffset(std::move(bus_label_offsets));

    renderer_settings.SetStopLabelFontSize(deserialized_settings.stop_label_font_size());

    std::vector<double> stop_label_offsets;
    for (double offset : deserialized_settings.stop_label_offset()) {
        stop_label_offsets.push_back(offset);
    }
    renderer_settings.SetStopLabelOffset(std::move(stop_label_offsets));

    renderer_settings.SetUnderlayerColor(detail::GetSvgColor(deserialized_settings.underlayer_color()));

    renderer_settings.SetUnderlayerWidth(deserialized_settings.underlayer_width());

    std::vector<svg::Color> color_palette;
    for (const route::serialize::Color& proto_color : deserialized_settings.color_palette()) {
        svg::Color color = GetSvgColor(proto_color);
        color_palette.push_back(std::move(color));
    }
    renderer_settings.SetColorPalette(std::move(color_palette));
}

// routing settings

route::serialize::RoutingSettings GetProtoRoutingSettings(const RoutingSettings& routing_settings) {
    route::serialize::RoutingSettings proto_settings;

    proto_settings.set_bus_wait_time(routing_settings.bus_wait_time);
    proto_settings.set_bus_velocity(routing_settings.bus_velocity);

    return proto_settings;
}

void DeserializeRoutingSettings(RoutingSettings& routing_settings, const route::serialize::RoutingSettings& deserialized_settings) {
    routing_settings.bus_wait_time = deserialized_settings.bus_wait_time();
    routing_settings.bus_velocity = deserialized_settings.bus_velocity();
}

route::serialize::TransportCatalogue GetProtoTCatalogue(const TransportCatalogue& catalogue, const RoutingSettings& routing_settings,
                                                        const renderer::MapSettings& map_settings) {
    route::serialize::TransportCatalogue proto_catalogue;

    // serialize all bus and stop names and get helper that is used to store next references of the names as ids
    const NameToId name_to_id = SerializeStopAndBusNames(proto_catalogue, catalogue);

    // serialize stops info
    for (const auto& [stop_name_sv, stop] : catalogue.GetAllStops()) {
        *proto_catalogue.add_stops() = GetProtoStop(stop_name_sv, stop, name_to_id);
    }

    // serialize buses info
    for (const Bus* bus : catalogue.GetAllBuses()) {
        *proto_catalogue.add_buses() = GetProtoBus(*bus, name_to_id);
    }

    // serialize distances between stops
    for (const auto& [from_sv, destination] : catalogue.GetAllDistances()) {
        for (const auto& [to_sv, distance] : destination) {
            *proto_catalogue.add_stop_stop_distances() = GetProtoStopToStopDistance(from_sv, to_sv, distance, name_to_id);
        }
    }

    // serialize routing and render settings
    *proto_catalogue.mutable_routing_settings() = GetProtoRoutingSettings(routing_settings);
    *proto_catalogue.mutable_render_settings() = GetProtoMapSettings(map_settings);

    return proto_catalogue;
}

}  // namespace detail

// Transport catalogue serialization

void SerializeTCatalogue(const TransportCatalogue& catalogue, const RoutingSettings& routing_settings,
                         const renderer::MapSettings& map_settings, const SerializationSettings& serialization_settings) {
    std::ofstream output(serialization_settings.db_path, std::ios::binary);

    route::serialize::TransportCatalogue proto_catalogue = detail::GetProtoTCatalogue(catalogue, routing_settings, map_settings);

    proto_catalogue.SerializeToOstream(&output);
}

void DeserializeTCatalogue(TransportCatalogue& catalogue, RoutingSettings& routing_settings,
                           renderer::MapSettings& map_settings, const route::serialize::TransportCatalogue& deserialized_data) {
    // 1. fill in transport catalogue from deserialized data

    // 1.1. add stops at first
    const auto& id_to_stop_name = deserialized_data.id_to_stop_name();
    for (const auto& proto_stop : deserialized_data.stops()) {
        geo::Coordinates coords{proto_stop.coordinates().latitude(), proto_stop.coordinates().longitude()};

        catalogue.AddStop(id_to_stop_name.at(proto_stop.stop_name_id()), coords);
    }

    // 1.2. then add distances between stops
    std::vector<Distance> distances;
    for (const auto& proto_dist : deserialized_data.stop_stop_distances()) {
        std::string_view from = id_to_stop_name.at(proto_dist.from_id());
        std::string_view to = id_to_stop_name.at(proto_dist.to_id());
        Distance d{from, to, static_cast<DistanceType>(proto_dist.distance())};

        distances.push_back(std::move(d));
    }
    catalogue.SetDistances(std::move(distances));

    // 1.3. and only after that add buses
    const auto& id_to_bus_name = deserialized_data.id_to_bus_name();
    for (const auto& proto_bus : deserialized_data.buses()) {
        std::vector<std::string> stops;
        for (const uint32_t& stop_id : proto_bus.stops()) {
            stops.push_back(std::string(id_to_stop_name.at(stop_id)));
        }
        catalogue.AddBus(id_to_bus_name.at(proto_bus.bus_name_id()), stops, !proto_bus.is_roundtrip());
    }

    // 2. deserialize settings

    detail::DeserializeMapSettings(map_settings, deserialized_data.render_settings());

    detail::DeserializeRoutingSettings(routing_settings, deserialized_data.routing_settings());
}

}  // namespace io
}  // namespace route
