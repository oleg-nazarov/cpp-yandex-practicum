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

route::serialize::Color GetSerColor(const svg::Color& color) {
    route::serialize::Color ser_color;

    if (std::holds_alternative<std::string>(color)) {
        ser_color.set_color_string(std::get<std::string>(color));
    } else if (std::holds_alternative<svg::Rgb>(color)) {
        svg::Rgb rgb_color = std::get<svg::Rgb>(color);

        route::serialize::ColorArray color_array;
        color_array.add_rgb(rgb_color.red);
        color_array.add_rgb(rgb_color.green);
        color_array.add_rgb(rgb_color.blue);

        *ser_color.mutable_color_array() = std::move(color_array);
    } else if (std::holds_alternative<svg::Rgba>(color)) {
        svg::Rgba rgba_color = std::get<svg::Rgba>(color);

        route::serialize::ColorArray color_array;
        color_array.add_rgb(rgba_color.red);
        color_array.add_rgb(rgba_color.green);
        color_array.add_rgb(rgba_color.blue);
        color_array.set_opacity(rgba_color.opacity);

        *ser_color.mutable_color_array() = std::move(color_array);
    }

    return ser_color;
}

svg::Color GetSvgColor(const route::serialize::Color& ser_color) {
    if (ser_color.has_color_string()) {
        svg::Color color{ser_color.color_string()};
        return color;
    } else if (ser_color.has_color_array()) {
        const auto& color_array = ser_color.color_array();

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

// map settings

route::serialize::RenderSettings GetSerializedMapSettings(const renderer::MapSettings& map_settings) {
    route::serialize::RenderSettings ser_renderer_settings;

    ser_renderer_settings.set_width(map_settings.GetWidth());
    ser_renderer_settings.set_height(map_settings.GetHeight());
    ser_renderer_settings.set_padding(map_settings.GetPadding());
    ser_renderer_settings.set_stop_radius(map_settings.GetStopRadius());
    ser_renderer_settings.set_line_width(map_settings.GetLineWidth());

    ser_renderer_settings.set_bus_label_font_size(map_settings.GetBusLabelFontSize());
    for (double bus_label_offset : map_settings.GetBusLabelOffset()) {
        ser_renderer_settings.add_bus_label_offset(bus_label_offset);
    }

    ser_renderer_settings.set_stop_label_font_size(map_settings.GetStopLabelFontSize());
    for (double stop_label_offset : map_settings.GetStopLabelOffset()) {
        ser_renderer_settings.add_stop_label_offset(stop_label_offset);
    }

    svg::Color underlayer_color = map_settings.GetUnderlayerColor();
    *ser_renderer_settings.mutable_underlayer_color() = GetSerColor(underlayer_color);

    ser_renderer_settings.set_underlayer_width(map_settings.GetUnderlayerWidth());

    for (const svg::Color& color_palette : map_settings.GetColorPalette()) {
        *ser_renderer_settings.add_color_palette() = GetSerColor(color_palette);
    }

    return ser_renderer_settings;
}

void DeserializeMapSettings(renderer::MapSettings& renderer_settings, const route::serialize::RenderSettings& deserialized_settings) {
    renderer_settings.SetWidth(deserialized_settings.width());
    renderer_settings.SetHeight(deserialized_settings.height());
    renderer_settings.SetPadding(deserialized_settings.padding());
    renderer_settings.SetStopRadius(deserialized_settings.stop_radius());
    renderer_settings.SetLineWidth(deserialized_settings.line_width());
    renderer_settings.SetBusLabelFontSize(deserialized_settings.bus_label_font_size());

    std::vector<double> bl_offsets;
    for (double bus_label_offset : deserialized_settings.bus_label_offset()) {
        bl_offsets.push_back(bus_label_offset);
    }
    renderer_settings.SetBusLabelOffset(std::move(bl_offsets));

    renderer_settings.SetStopLabelFontSize(deserialized_settings.stop_label_font_size());

    std::vector<double> sl_offsets;
    for (double stop_label_offset : deserialized_settings.stop_label_offset()) {
        sl_offsets.push_back(stop_label_offset);
    }
    renderer_settings.SetStopLabelOffset(std::move(sl_offsets));

    renderer_settings.SetUnderlayerColor(detail::GetSvgColor(deserialized_settings.underlayer_color()));

    renderer_settings.SetUnderlayerWidth(deserialized_settings.underlayer_width());

    std::vector<svg::Color> color_palette;
    for (const route::serialize::Color& ser_color : deserialized_settings.color_palette()) {
        svg::Color color = GetSvgColor(ser_color);
        color_palette.push_back(std::move(color));
    }
    renderer_settings.SetColorPalette(std::move(color_palette));
}

// routing settings

route::serialize::RoutingSettings GetSerializedRoutingSettings(const RoutingSettings& routing_settings) {
    route::serialize::RoutingSettings ser_settings;

    ser_settings.set_bus_wait_time(routing_settings.bus_wait_time);
    ser_settings.set_bus_velocity(routing_settings.bus_velocity);

    return ser_settings;
}

void DeserializeRoutingSettings(RoutingSettings& routing_settings, const route::serialize::RoutingSettings& deserialized_settings) {
    routing_settings.bus_wait_time = deserialized_settings.bus_wait_time();
    routing_settings.bus_velocity = deserialized_settings.bus_velocity();
}

}  // namespace detail

// Transport catalogue serialization

void SerializeTCatalogue(const TransportCatalogue& catalogue, const RoutingSettings& routing_settings,
                         const renderer::MapSettings& map_settings, const SerializationSettings& serialization_settings) {
    std::ofstream output(serialization_settings.db_path, std::ios::binary);

    route::serialize::TransportCatalogue ser_catalogue;

    // we create these two structures (only for local purposes) for not having in .proto string
    // duplicates and storing uint32_t
    std::unordered_map<std::string_view, uint32_t> stop_name_to_id;
    std::unordered_map<std::string_view, uint32_t> bus_name_to_id;

    // serialize stop names
    auto& ser_id_to_stop_name = *ser_catalogue.mutable_id_to_stop_name();
    const std::unordered_map<std::string_view, Stop>& stops = catalogue.GetAllStops();
    for (const auto& [stop_name_sv, _] : stops) {
        uint32_t id = ser_id_to_stop_name.size();

        ser_id_to_stop_name[id] = std::string(stop_name_sv);
        stop_name_to_id[stop_name_sv] = id;
    }

    // serialize bus names
    auto& ser_id_to_bus_name = *ser_catalogue.mutable_id_to_bus_name();
    const std::vector<const Bus*> buses = catalogue.GetAllBuses();
    for (const Bus* bus : buses) {
        uint32_t id = ser_id_to_bus_name.size();

        ser_id_to_bus_name[id] = std::string(bus->name);
        bus_name_to_id[bus->name] = id;
    }

    // after we have ids of all stops and buses (stop_name_to_id and bus_name_to_id),
    // we can finish serializing other data

    // serialize stops info
    auto& ser_stops = *ser_catalogue.mutable_stops();
    for (const auto& [stop_name_sv, stop] : stops) {
        uint32_t id = stop_name_to_id[stop_name_sv];

        route::serialize::Stop ser_stop;
        ser_stop.set_latitude(stop.coordinates.lat);
        ser_stop.set_longitude(stop.coordinates.lng);

        for (std::string_view bus_sv : stop.buses) {
            ser_stop.add_buses(bus_name_to_id[bus_sv]);
        }

        ser_stops[id] = std::move(ser_stop);
    }

    // serialize buses info
    auto& ser_buses = *ser_catalogue.mutable_buses();
    for (const Bus* bus : buses) {
        uint32_t id = bus_name_to_id[bus->name];

        route::serialize::Bus ser_bus;

        // we store stops in transport catalogue in a way that represents a roundtrip way
        // (no matter what "is_roundtrip"-flag actually is);
        // so if the flag is "false", we should serialize only half+1 of the stops (one way stops)
        auto it_end = bus->is_roundtrip ? bus->stops.end() : bus->stops.begin() + (bus->stops.size() / 2) + 1;
        for (auto it = bus->stops.begin(); it != it_end; ++it) {
            ser_bus.add_stops(stop_name_to_id[*it]);
        }

        ser_bus.set_is_roundtrip(bus->is_roundtrip);

        ser_buses[id] = std::move(ser_bus);
    }

    // serialize distances between stops
    auto& ser_distance = *ser_catalogue.mutable_stop_stop_distance();
    const std::unordered_map<std::string_view, std::unordered_map<std::string_view, DistanceType>>&
        distances = catalogue.GetAllDistances();
    for (const auto& [from_sv, destination] : distances) {
        route::serialize::DestinationDistance ser_destination_distance;
        for (const auto& [to_sv, distance] : destination) {
            uint32_t to_id = stop_name_to_id[to_sv];

            (*ser_destination_distance.mutable_distance())[to_id] = distance;
        }
        uint32_t from_id = stop_name_to_id[from_sv];
        ser_distance[from_id] = std::move(ser_destination_distance);
    }

    // serialize settings

    route::serialize::RoutingSettings ser_routing_settings = detail::GetSerializedRoutingSettings(routing_settings);
    route::serialize::RenderSettings ser_map_settings = detail::GetSerializedMapSettings(map_settings);

    route::serialize::Settings settings;
    *settings.mutable_routing() = std::move(ser_routing_settings);
    *settings.mutable_rendering() = std::move(ser_map_settings);

    *ser_catalogue.mutable_settings() = std::move(settings);

    // finish
    ser_catalogue.SerializeToOstream(&output);
}

void DeserializeTCatalogue(TransportCatalogue& catalogue, RoutingSettings& routing_settings,
                           renderer::MapSettings& map_settings, const SerializationSettings& serialization_settings) {
    std::ifstream input(serialization_settings.db_path, std::ios::binary);
    route::serialize::TransportCatalogue deserialized_data;
    deserialized_data.ParseFromIstream(&input);

    // 1. fill in transport catalogue from deserialized data

    // 1.1. add stops at first
    const auto& id_to_stop_name = deserialized_data.id_to_stop_name();
    for (const auto& [stop_name_id, stop] : deserialized_data.stops()) {
        geo::Coordinates coords{stop.latitude(), stop.longitude()};

        catalogue.AddStop(id_to_stop_name.at(stop_name_id), coords);
    }

    // 1.2. then add distances between stops
    std::vector<Distance> distances;
    const auto& stop_stop_distance = deserialized_data.stop_stop_distance();
    for (const auto& [from_id, destination_distance] : stop_stop_distance) {
        for (const auto& [to_id, distance] : destination_distance.distance()) {
            std::string_view from = id_to_stop_name.at(from_id);
            std::string_view to = id_to_stop_name.at(to_id);
            Distance d{from, to, static_cast<DistanceType>(distance)};
            distances.push_back(std::move(d));
        }
    }
    catalogue.SetDistances(std::move(distances));

    // 1.3. and only after that add buses
    const auto& id_to_bus_name = deserialized_data.id_to_bus_name();
    for (const auto& [bus_name_id, bus] : deserialized_data.buses()) {
        std::vector<std::string> stops;
        for (const uint32_t& stop_id : bus.stops()) {
            stops.push_back(std::string(id_to_stop_name.at(stop_id)));
        }
        catalogue.AddBus(id_to_bus_name.at(bus_name_id), stops, !bus.is_roundtrip());
    }

    // 2. deserialize settings

    detail::DeserializeMapSettings(map_settings, deserialized_data.settings().rendering());

    detail::DeserializeRoutingSettings(routing_settings, deserialized_data.settings().routing());
}

}  // namespace io
}  // namespace route
