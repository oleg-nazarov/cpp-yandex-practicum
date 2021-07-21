#pragma once

#include <iostream>
#include <optional>
#include <string>

#include "json/json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

namespace route {
namespace io {

class RequestHandler {
   public:
    RequestHandler(TransportCatalogue& catalogue);
    RequestHandler(renderer::MapRenderer& map_renderer);
    RequestHandler(renderer::MapSettings& map_settings);
    RequestHandler(TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer);

    virtual void Handle(const json::Node& node) const = 0;

    virtual const std::string& GetRequestType() const = 0;

   protected:
    ~RequestHandler() = default;

    std::optional<TransportCatalogue* const> catalogue_;
    std::optional<renderer::MapRenderer* const> map_renderer_;
    std::optional<renderer::MapSettings* const> map_settings_;
};

class AddDataHandler : public RequestHandler {
   public:
    AddDataHandler(TransportCatalogue& catalogue);

    void Handle(const json::Node& node) const override;

    const std::string& GetRequestType() const override;

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

    void HandleAddBus(const json::Dict& node) const;
    void HandleAddStop(const json::Dict& node) const;
};

class GetDataHandler : public RequestHandler {
   public:
    GetDataHandler(std::ostream& out, TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer);

    void Handle(const json::Node& node) const override;

    const std::string& GetRequestType() const override;

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

    json::Node GetBusInfoResponse(const json::Node& node) const;

    json::Node GetStopInfoResponse(const json::Node& node) const;

    json::Node GetMapResponse(const json::Node& node) const;
};

class SetMapSettingsHandler : public RequestHandler {
   public:
    SetMapSettingsHandler(renderer::MapSettings& map_settings);

    void Handle(const json::Node& node) const override;

    const std::string& GetRequestType() const override;

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

    void MatchSettingKeyToHandler(const std::string& key, const json::Node& node) const;

    std::vector<double> GetOffsetsFromNode(const json::Node& node) const;
    svg::Color GetColorFromNode(const json::Node& node) const;
};

}  // namespace io
}  // namespace route
