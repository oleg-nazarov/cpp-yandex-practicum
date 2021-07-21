#include "request_handler.h"

#include <iterator>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

#include "geo.h"
#include "json/json.h"
#include "map_renderer.h"

namespace route {
namespace io {

RequestHandler::RequestHandler(TransportCatalogue& catalogue) : catalogue_(&catalogue) {}
RequestHandler::RequestHandler(renderer::MapRenderer& map_renderer) : map_renderer_(&map_renderer) {}
RequestHandler::RequestHandler(renderer::MapSettings& map_settings) : map_settings_(&map_settings) {}
RequestHandler::RequestHandler(TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer)
    : catalogue_(&catalogue), map_renderer_(&map_renderer) {}

// ---------- AddDataHandler ----------

AddDataHandler::AddDataHandler(TransportCatalogue& catalogue) : RequestHandler(catalogue) {}

void AddDataHandler::Handle(const json::Node& node) const {
    using namespace std::literals::string_literals;

    const json::Array& requests = node.AsArray();

    std::queue<const json::Dict*> buses_queue;

    for (const json::Node& req_node : requests) {
        const json::Dict& request_map = req_node.AsMap();

        // we handle info about buses after we've handled data about stops, because buses
        // use a stop's latitude and longitude to calculate the route distance
        if (request_map.at(AddDataHandler::TYPE_KEY) == AddDataHandler::BUS_TYPE) {
            buses_queue.push(&request_map);

            continue;
        }

        if (request_map.at(AddDataHandler::TYPE_KEY) != AddDataHandler::STOP_TYPE) {
            throw std::logic_error("Unknown request type"s);
        }

        HandleAddStop(request_map);
    }

    // handle the info about buses
    while (!buses_queue.empty()) {
        HandleAddBus(*(buses_queue.front()));
        buses_queue.pop();
    }
}

void AddDataHandler::HandleAddBus(const json::Dict& req_map) const {
    std::vector<std::string> stops;
    for (const json::Node& stop_node : req_map.at(AddDataHandler::STOPS_KEY).AsArray()) {
        stops.push_back(std::move(stop_node.AsString()));
    }

    (*catalogue_)->AddBus(req_map.at(AddDataHandler::NAME_KEY).AsString(), stops, !req_map.at(AddDataHandler::ROUNDTRIP_KEY).AsBool());
}

void AddDataHandler::HandleAddStop(const json::Dict& req_map) const {
    // add stop
    geo::Coordinates coords{
        req_map.at(AddDataHandler::LATITUDE_KEY).AsDouble(),
        req_map.at(AddDataHandler::LONGITUDE_KEY).AsDouble()};

    (*catalogue_)->AddStop(req_map.at(AddDataHandler::NAME_KEY).AsString(), coords);

    // set distances
    std::vector<Distance> distances;
    for (const auto& [to_s, distance_node] : req_map.at(AddDataHandler::DISTANCES_KEY).AsMap()) {
        Distance dist{
            req_map.at(AddDataHandler::NAME_KEY).AsString(),
            to_s,
            static_cast<unsigned long long>(distance_node.AsDouble())};

        distances.push_back(std::move(dist));
    }

    (*catalogue_)->SetDistances(std::move(distances));
}

const std::string& AddDataHandler::GetRequestType() const {
    return REQUEST_TYPE;
}

// ---------- GetDataHandler ----------

GetDataHandler::GetDataHandler(std::ostream& out, TransportCatalogue& catalogue,
                               renderer::MapRenderer& map_renderer)
    : RequestHandler(catalogue, map_renderer), out_(out) {}

void GetDataHandler::Handle(const json::Node& node) const {
    // create json-node
    const json::Array& requests = node.AsArray();
    std::vector<json::Node> responses;
    responses.reserve(requests.size());

    for (const json::Node& req_node : requests) {
        using namespace std::literals::string_literals;

        const std::string& request_type = req_node.AsMap().at(GetDataHandler::TYPE_KEY).AsString();

        if (request_type == GetDataHandler::BUS_TYPE) {
            responses.push_back(GetBusInfoResponse(req_node));
        } else if (request_type == GetDataHandler::STOP_TYPE) {
            responses.push_back(GetStopInfoResponse(req_node));
        } else if (request_type == GetDataHandler::MAP_TYPE) {
            responses.push_back(GetMapResponse(req_node));
        } else {
            throw std::logic_error("Unknown request type"s);
        }
    }

    // output json-node
    json::Node root(responses);
    json::Document doc(root);

    json::Print(doc, out_);
}

const std::string& GetDataHandler::GetRequestType() const {
    return REQUEST_TYPE;
}

json::Node GetDataHandler::GetBusInfoResponse(const json::Node& node) const {
    json::Dict response;
    const json::Dict& request = node.AsMap();

    // fill request id:
    response[GetDataHandler::REQUEST_ID_KEY] = request.at(GetDataHandler::ID_KEY);

    // fill other info
    if (auto info = (*catalogue_)->GetBusInfo(request.at(GetDataHandler::NAME_KEY).AsString())) {
        response[GetDataHandler::CURVATURE_KEY] = json::Node((*info).curvature);

        response[GetDataHandler::ROUTE_LENGTH_KEY] = json::Node(static_cast<int>((*info).road_distance));

        response[GetDataHandler::STOP_COUNT_KEY] = json::Node(static_cast<int>((*info).stops_count));

        response[GetDataHandler::UNIQUE_STOP_COUNT_KEY] = json::Node(static_cast<int>((*info).unique_stops_count));
    } else {
        response[GetDataHandler::ERROR_MESSAGE_KEY] = json::Node(GetDataHandler::NOT_FOUND_S);
    }

    return json::Node(response);
}

json::Node GetDataHandler::GetStopInfoResponse(const json::Node& node) const {
    json::Dict response;
    const json::Dict& request = node.AsMap();

    // fill request id
    response[GetDataHandler::REQUEST_ID_KEY] = request.at(GetDataHandler::ID_KEY);

    // fill buses array
    if (auto info = (*catalogue_)->GetBusesByStop(request.at(GetDataHandler::NAME_KEY).AsString())) {
        json::Array buses_arr;

        for (std::string_view bus_name_sv : *info) {
            buses_arr.push_back(json::Node(std::string(bus_name_sv)));
        }

        response[GetDataHandler::BUSES_KEY] = json::Node(buses_arr);
    } else {
        response[GetDataHandler::ERROR_MESSAGE_KEY] = json::Node(GetDataHandler::NOT_FOUND_S);
    }

    return json::Node(response);
}

json::Node GetDataHandler::GetMapResponse(const json::Node& node) const {
    json::Dict response;
    const json::Dict& request = node.AsMap();

    response[GetDataHandler::REQUEST_ID_KEY] = request.at(GetDataHandler::ID_KEY);

    const std::string map_svg_s = (*map_renderer_)->GetMapSVG((*catalogue_)->GetAllBuses(), (*catalogue_)->GetAllStops());
    response[GetDataHandler::MAP_KEY] = json::Node(map_svg_s);

    return json::Node(response);
}

// ---------- SetMapSettingsHandler ----------

SetMapSettingsHandler::SetMapSettingsHandler(renderer::MapSettings& map_settings)
    : RequestHandler(map_settings) {}

void SetMapSettingsHandler::Handle(const json::Node& node) const {
    const json::Dict& settings_map = node.AsMap();

    for (const auto& [key_s, value_node] : settings_map) {
        MatchSettingKeyToHandler(key_s, value_node);
    }
}

const std::string& SetMapSettingsHandler::GetRequestType() const {
    return REQUEST_TYPE;
}

void SetMapSettingsHandler::MatchSettingKeyToHandler(const std::string& key, const json::Node& node) const {
    using namespace std::literals;

    if (key == WIDTH_KEY) {
        (*map_settings_)->SetWidth(node.AsDouble());
    } else if (key == HEIGHT_KEY) {
        (*map_settings_)->SetHeight(node.AsDouble());
    } else if (key == PADDING_KEY) {
        (*map_settings_)->SetPadding(node.AsDouble());
    } else if (key == LINE_WIDTH_KEY) {
        (*map_settings_)->SetLineWidth(node.AsDouble());
    } else if (key == STOP_RADIUS_KEY) {
        (*map_settings_)->SetStopRadius(node.AsDouble());
    } else if (key == BUS_LABEL_FONT_SIZE_KEY) {
        (*map_settings_)->SetBusLabelFontSize(node.AsInt());
    } else if (key == BUS_LABEL_OFFSET_KEY) {
        (*map_settings_)->SetBusLabelOffset(GetOffsetsFromNode(node));
    } else if (key == STOP_LABEL_FONT_SIZE_KEY) {
        (*map_settings_)->SetStopLabelFontSize(node.AsInt());
    } else if (key == STOP_LABEL_OFFSET_KEY) {
        (*map_settings_)->SetStopLabelOffset(GetOffsetsFromNode(node));
    } else if (key == UNDERLAYER_COLOR_KEY) {
        (*map_settings_)->SetUnderlayerColor(GetColorFromNode(node));
    } else if (key == UNDERLAYER_WIDTH_KEY) {
        (*map_settings_)->SetUnderlayerWidth(node.AsDouble());
    } else if (key == COLOR_PALETTE_KEY) {
        std::vector<svg::Color> colors(node.AsArray().size());
        std::transform(
            std::execution::par,
            node.AsArray().begin(), node.AsArray().end(),
            colors.begin(),
            [&](const json::Node& node) {
                return GetColorFromNode(node);
            });

        (*map_settings_)->SetColorPalette(std::move(colors));
    } else {
        throw std::logic_error("Unknown setting"s);
    }
}

std::vector<double> SetMapSettingsHandler::GetOffsetsFromNode(const json::Node& node) const {
    std::vector<double> offsets(node.AsArray().size());
    std::transform(
        std::execution::par,
        node.AsArray().begin(), node.AsArray().end(),
        offsets.begin(),
        [](const auto& node) {
            return node.AsDouble();
        });

    return offsets;
}
svg::Color SetMapSettingsHandler::GetColorFromNode(const json::Node& node) const {
    if (node.IsString()) {
        return node.AsString();
    } else if (node.IsArray()) {
        const json::Array& arr = node.AsArray();

        svg::Color c;
        if (arr.size() == 3u) {
            c = svg::Rgb(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt());
        } else {
            c = svg::Rgba(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt(), arr[3].AsDouble());
        }

        return c;
    }

    return nullptr;
}

}  // namespace io
}  // namespace route
