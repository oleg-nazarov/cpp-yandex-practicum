#include "json_reader.h"

#include <iterator>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

#include "domain.h"
#include "geo.h"
#include "json/json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

namespace route {
namespace io {

namespace detail {

// ---------- RequestHandler ----------

class RequestHandler {
   public:
    RequestHandler(TransportCatalogue& catalogue) : catalogue_(&catalogue) {}
    RequestHandler(renderer::MapRenderer& map_renderer) : map_renderer_(&map_renderer) {}
    RequestHandler(renderer::MapSettings& map_settings) : map_settings_(&map_settings) {}
    RequestHandler(TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer)
        : catalogue_(&catalogue), map_renderer_(&map_renderer) {}

    virtual void Handle(const json::Node& node) const = 0;

    virtual const std::string& GetRequestType() const = 0;

   protected:
    ~RequestHandler() = default;

    std::optional<TransportCatalogue* const> catalogue_;
    std::optional<renderer::MapRenderer* const> map_renderer_;
    std::optional<renderer::MapSettings* const> map_settings_;
};

void HandleRequests(const json::Node& json_node, const std::vector<const RequestHandler*>& handlers) {
    const json::Dict& json_map = json_node.AsMap();

    for (const RequestHandler* handler : handlers) {
        const json::Node& request_node = json_map.at(handler->GetRequestType());
        handler->Handle(request_node);
    }
}

// ---------- AddDataHandler ----------

class AddDataHandler : public RequestHandler {
   public:
    AddDataHandler(TransportCatalogue& catalogue) : RequestHandler(catalogue) {}

    void Handle(const json::Node& node) const override {
        using namespace std::literals::string_literals;

        const json::Array& requests = node.AsArray();

        std::queue<const json::Dict*> buses_queue;

        for (const json::Node& req_node : requests) {
            const json::Dict& request_map = req_node.AsMap();

            // we handle info about buses after we've handled data about stops, because buses
            // use a stop's latitude and longitude to calculate the route distance
            if (request_map.at(TYPE_KEY).AsString() == BUS_TYPE) {
                buses_queue.push(&request_map);

                continue;
            }

            if (request_map.at(TYPE_KEY).AsString() != STOP_TYPE) {
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

    const std::string& GetRequestType() const override {
        return REQUEST_TYPE;
    }

   private:
    inline static const std::string REQUEST_TYPE = "base_requests";

    inline static const std::string DISTANCES_KEY = "road_distances";
    inline static const std::string LATITUDE_KEY = "latitude";
    inline static const std::string LONGITUDE_KEY = "longitude";
    inline static const std::string NAME_KEY = "name";
    inline static const std::string ROUNDTRIP_KEY = "is_roundtrip";
    inline static const std::string STOPS_KEY = "stops";
    inline static const std::string TYPE_KEY = "type";

    inline static const std::string STOP_TYPE = "Stop";
    inline static const std::string BUS_TYPE = "Bus";

    void HandleAddBus(const json::Dict& req_map) const {
        std::vector<std::string> stops;
        for (const json::Node& stop_node : req_map.at(STOPS_KEY).AsArray()) {
            stops.push_back(std::move(stop_node.AsString()));
        }

        (*catalogue_)->AddBus(req_map.at(NAME_KEY).AsString(), stops, !req_map.at(AddDataHandler::ROUNDTRIP_KEY).AsBool());
    }

    void HandleAddStop(const json::Dict& req_map) const {
        // add stop
        geo::Coordinates coords{
            req_map.at(LATITUDE_KEY).AsDouble(),
            req_map.at(LONGITUDE_KEY).AsDouble()};

        (*catalogue_)->AddStop(req_map.at(NAME_KEY).AsString(), coords);

        // set distances
        std::vector<Distance> distances;
        for (const auto& [to_s, distance_node] : req_map.at(DISTANCES_KEY).AsMap()) {
            Distance dist{
                req_map.at(NAME_KEY).AsString(),
                to_s,
                static_cast<unsigned long long>(distance_node.AsDouble())};

            distances.push_back(std::move(dist));
        }

        (*catalogue_)->SetDistances(std::move(distances));
    }
};

// ---------- GetDataHandler ----------

class GetDataHandler : public RequestHandler {
   public:
    GetDataHandler(std::ostream& out, TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer)
        : RequestHandler(catalogue, map_renderer), out_(out) {}

    void Handle(const json::Node& node) const override {
        // create json-node
        const json::Array& requests = node.AsArray();
        std::vector<json::Node> responses;
        responses.reserve(requests.size());

        for (const json::Node& req_node : requests) {
            using namespace std::literals::string_literals;

            const std::string& request_type = req_node.AsMap().at(TYPE_KEY).AsString();

            if (request_type == BUS_TYPE) {
                responses.push_back(GetBusInfoResponse(req_node));
            } else if (request_type == STOP_TYPE) {
                responses.push_back(GetStopInfoResponse(req_node));
            } else if (request_type == MAP_TYPE) {
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

    const std::string& GetRequestType() const override {
        return REQUEST_TYPE;
    }

   private:
    std::ostream& out_;

    inline static const std::string REQUEST_TYPE = "stat_requests";

    inline static const std::string BUSES_KEY = "buses";
    inline static const std::string CURVATURE_KEY = "curvature";
    inline static const std::string ERROR_MESSAGE_KEY = "error_message";
    inline static const std::string ID_KEY = "id";
    inline static const std::string MAP_KEY = "map";
    inline static const std::string NAME_KEY = "name";
    inline static const std::string REQUEST_ID_KEY = "request_id";
    inline static const std::string ROUTE_LENGTH_KEY = "route_length";
    inline static const std::string STOP_COUNT_KEY = "stop_count";
    inline static const std::string TYPE_KEY = "type";
    inline static const std::string UNIQUE_STOP_COUNT_KEY = "unique_stop_count";

    inline static const std::string BUS_TYPE = "Bus";
    inline static const std::string MAP_TYPE = "Map";
    inline static const std::string STOP_TYPE = "Stop";

    inline static const std::string NOT_FOUND_S = "not found";

    json::Node GetBusInfoResponse(const json::Node& node) const {
        json::Dict response;
        const json::Dict& request = node.AsMap();

        // fill request id:
        response[REQUEST_ID_KEY] = request.at(ID_KEY);

        // fill other info
        if (auto info = (*catalogue_)->GetBusInfo(request.at(NAME_KEY).AsString())) {
            response[CURVATURE_KEY] = json::Node((*info).curvature);

            response[ROUTE_LENGTH_KEY] = json::Node(static_cast<int>((*info).road_distance));

            response[STOP_COUNT_KEY] = json::Node(static_cast<int>((*info).stops_count));

            response[UNIQUE_STOP_COUNT_KEY] = json::Node(static_cast<int>((*info).unique_stops_count));
        } else {
            response[ERROR_MESSAGE_KEY] = json::Node(NOT_FOUND_S);
        }

        return json::Node(response);
    }

    json::Node GetStopInfoResponse(const json::Node& node) const {
        json::Dict response;
        const json::Dict& request = node.AsMap();

        // fill request id
        response[REQUEST_ID_KEY] = request.at(ID_KEY);

        // fill buses array
        if (auto info = (*catalogue_)->GetBusesByStop(request.at(NAME_KEY).AsString())) {
            json::Array buses_arr;

            for (std::string_view bus_name_sv : *(*info)) {
                buses_arr.push_back(json::Node(std::string(bus_name_sv)));
            }

            response[BUSES_KEY] = json::Node(buses_arr);
        } else {
            response[ERROR_MESSAGE_KEY] = json::Node(NOT_FOUND_S);
        }

        return json::Node(response);
    }

    json::Node GetMapResponse(const json::Node& node) const {
        json::Dict response;
        const json::Dict& request = node.AsMap();

        response[REQUEST_ID_KEY] = request.at(ID_KEY);

        const std::string map_svg_s = (*map_renderer_)->GetMapSVG((*catalogue_)->GetAllBuses(), (*catalogue_)->GetAllStops());
        response[MAP_KEY] = json::Node(map_svg_s);

        return json::Node(response);
    }
};

// ---------- SetMapSettingsHandler ----------

class SetMapSettingsHandler : public RequestHandler {
   public:
    SetMapSettingsHandler(renderer::MapSettings& map_settings) : RequestHandler(map_settings) {}

    void Handle(const json::Node& node) const override {
        const json::Dict& settings_map = node.AsMap();

        for (const auto& [key_s, value_node] : settings_map) {
            MatchSettingKeyToHandler(key_s, value_node);
        }
    }

    const std::string& GetRequestType() const override {
        return REQUEST_TYPE;
    }

   private:
    inline static const std::string REQUEST_TYPE = "render_settings";

    inline static const std::string WIDTH_KEY = "width";
    inline static const std::string HEIGHT_KEY = "height";
    inline static const std::string PADDING_KEY = "padding";
    inline static const std::string LINE_WIDTH_KEY = "line_width";
    inline static const std::string STOP_RADIUS_KEY = "stop_radius";
    inline static const std::string BUS_LABEL_FONT_SIZE_KEY = "bus_label_font_size";
    inline static const std::string BUS_LABEL_OFFSET_KEY = "bus_label_offset";
    inline static const std::string STOP_LABEL_FONT_SIZE_KEY = "stop_label_font_size";
    inline static const std::string STOP_LABEL_OFFSET_KEY = "stop_label_offset";
    inline static const std::string UNDERLAYER_COLOR_KEY = "underlayer_color";
    inline static const std::string UNDERLAYER_WIDTH_KEY = "underlayer_width";
    inline static const std::string COLOR_PALETTE_KEY = "color_palette";

    void MatchSettingKeyToHandler(const std::string& key, const json::Node& node) const {
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

    std::vector<double> GetOffsetsFromNode(const json::Node& node) const {
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

    svg::Color GetColorFromNode(const json::Node& node) const {
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
};

}  // namespace detail

void ReadJSON(std::istream& input, std::ostream& output, TransportCatalogue& catalogue) {
    const json::Document document = json::Load(input);
    const json::Node& json_node = document.GetRoot();

    // handle map settings before creating map_renderer
    renderer::MapSettings map_settings;
    detail::SetMapSettingsHandler settings_h(map_settings);
    detail::HandleRequests(json_node, {&settings_h});

    // handle other requests
    route::renderer::MapRenderer map_renderer(std::move(map_settings));
    detail::AddDataHandler add_h(catalogue);
    detail::GetDataHandler get_h(output, catalogue, map_renderer);

    // order is important: firstlly - add data, only then - get new information
    std::vector<const detail::RequestHandler*> handlers{&add_h, &get_h};
    detail::HandleRequests(json_node, handlers);
}

}  // namespace io
}  // namespace route