#include "map_renderer.h"

#include <algorithm>
#include <cassert>
#include <execution>
#include <iterator>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "domain.h"
#include "svg/svg.h"

namespace route {
namespace renderer {

namespace detail {

// ---------- RouteName ----------

RouteName::RouteName(std::string_view name, const svg::Point& point, const MapSettings& settings)
    : name_(name), point_(point), settings_(settings) {}

svg::Text RouteName::GetObject() const {
    return svg::Text()
        .SetPosition(point_)
        .SetOffset({settings_.GetBusLabelOffset()[0], settings_.GetBusLabelOffset()[1]})
        .SetFontSize(settings_.GetBusLabelFontSize())
        .SetFontFamily(font_family_)
        .SetFontWeight(font_weight_)
        .SetData(std::string(name_));
}

RouteNameText::RouteNameText(std::string_view name, int idx_color_palette, const svg::Point& point,
                             const MapSettings& settings)
    : RouteName(name, point, settings), fill_color_(settings.GetColorPalette()[idx_color_palette]) {}

svg::Text RouteNameText::GetObject() const {
    return RouteName::GetObject().SetFillColor(fill_color_);
}

RouteNameUnderlayer::RouteNameUnderlayer(std::string_view name, const svg::Point& point,
                                         const MapSettings& settings)
    : RouteName(name, point, settings),
      fill_color_(settings.GetUnderlayerColor()),
      stroke_color_(settings.GetUnderlayerColor()),
      stroke_width_(settings.GetUnderlayerWidth()) {}

svg::Text RouteNameUnderlayer::GetObject() const {
    return RouteName::GetObject()
        .SetFillColor(fill_color_)
        .SetStrokeColor(stroke_color_)
        .SetStrokeWidth(stroke_width_)
        .SetStrokeLineCap(stroke_line_cap_)
        .SetStrokeLineJoin(stroke_line_join_);
}

// ---------- StopName ----------

StopName::StopName(std::string_view name, const svg::Point& point, const MapSettings& settings)
    : name_(name), point_(point), settings_(settings) {}

svg::Text StopName::GetObject() const {
    return svg::Text()
        .SetPosition(point_)
        .SetOffset({settings_.GetStopLabelOffset()[0], settings_.GetStopLabelOffset()[1]})
        .SetFontSize(settings_.GetStopLabelFontSize())
        .SetFontFamily(font_family_)
        .SetData(std::string(name_));
}

StopNameText::StopNameText(std::string_view name, const svg::Point& point, const MapSettings& settings)
    : StopName(name, point, settings) {}

svg::Text StopNameText::GetObject() const {
    return StopName::GetObject().SetFillColor(fill_color_);
}

StopNameUnderlayer::StopNameUnderlayer(std::string_view name, const svg::Point& point,
                                       const MapSettings& settings)
    : StopName(name, point, settings),
      fill_color_(settings.GetUnderlayerColor()),
      stroke_color_(settings.GetUnderlayerColor()),
      stroke_width_(settings.GetUnderlayerWidth()) {}

svg::Text StopNameUnderlayer::GetObject() const {
    return StopName::GetObject()
        .SetFillColor(fill_color_)
        .SetStrokeColor(stroke_color_)
        .SetStrokeWidth(stroke_width_)
        .SetStrokeLineCap(stroke_line_cap_)
        .SetStrokeLineJoin(stroke_line_join_);
}

}  // namespace detail

// ---------- MapSettings ----------

void MapSettings::SetWidth(double width) {
    width_ = width;
}
double MapSettings::GetWidth() const noexcept {
    return width_;
}

void MapSettings::SetHeight(double height) {
    height_ = height;
}
double MapSettings::GetHeight() const noexcept {
    return height_;
}

void MapSettings::SetPadding(double padding) {
    padding_ = padding;
}
double MapSettings::GetPadding() const noexcept {
    return padding_;
}

void MapSettings::SetLineWidth(double line_width) {
    line_width_ = line_width;
}
double MapSettings::GetLineWidth() const noexcept {
    return line_width_;
}

void MapSettings::SetStopRadius(double stop_radius) {
    stop_radius_ = stop_radius;
}
double MapSettings::GetStopRadius() const noexcept {
    return stop_radius_;
}

void MapSettings::SetBusLabelFontSize(int bus_label_font_size) {
    bus_label_font_size_ = bus_label_font_size;
}
int MapSettings::GetBusLabelFontSize() const noexcept {
    return bus_label_font_size_;
}

void MapSettings::SetBusLabelOffset(std::vector<double>&& bus_label_offset) {
    bus_label_offset_ = std::move(bus_label_offset);
}
std::vector<double> MapSettings::GetBusLabelOffset() const noexcept {
    return bus_label_offset_;
}

void MapSettings::SetStopLabelFontSize(int stop_label_font_size) {
    stop_label_font_size_ = stop_label_font_size;
}
int MapSettings::GetStopLabelFontSize() const noexcept {
    return stop_label_font_size_;
}

void MapSettings::SetStopLabelOffset(std::vector<double>&& stop_label_offset) {
    stop_label_offset_ = std::move(stop_label_offset);
}
std::vector<double> MapSettings::GetStopLabelOffset() const noexcept {
    return stop_label_offset_;
}

void MapSettings::SetUnderlayerColor(svg::Color&& underlayer_color) {
    underlayer_color_ = std::move(underlayer_color);
}
svg::Color MapSettings::GetUnderlayerColor() const noexcept {
    return underlayer_color_;
}

void MapSettings::SetUnderlayerWidth(double underlayer_width) {
    underlayer_width_ = underlayer_width;
}
double MapSettings::GetUnderlayerWidth() const noexcept {
    return underlayer_width_;
}

void MapSettings::SetColorPalette(std::vector<svg::Color>&& color_palette) {
    color_palette_ = std::move(color_palette);
}
std::vector<svg::Color> MapSettings::GetColorPalette() const noexcept {
    return color_palette_;
}

// ---------- MapRenderer ----------

MapRenderer::MapRenderer(MapSettings&& settings) : settings_(std::move(settings)) {}

std::string MapRenderer::GetMapSVG(std::vector<const Bus*> buses,
                                   const std::unordered_map<std::string_view, Stop>& stops) {
    SetUpProjector(stops);

    // sort bus names in ascending order
    std::sort(
        std::execution::par,
        buses.begin(), buses.end(),
        [](const Bus* lhs, const Bus* rhs) {
            return lhs->name < rhs->name;
        });

    // add svg object
    svg::Document doc;

    AddLinesBetweenStops(doc, buses, stops);
    AddRouteNames(doc, buses, stops);
    AddStopSymbols(doc, stops);
    AddStopNames(doc, stops);

    // output
    std::ostringstream out;
    doc.Render(out);

    return out.str();
}

int MapRenderer::GetNextColorPaletteIdx(int cuurent_idx) const noexcept {
    return (cuurent_idx + 1) % settings_.GetColorPalette().size();
}

void MapRenderer::SetUpProjector(const std::unordered_map<std::string_view, Stop>& stops) {
    std::vector<geo::Coordinates> stop_coords;
    stop_coords.reserve(stops.size());

    for (const auto& p : stops) {
        if (p.second.buses.empty()) {
            continue;
        }

        stop_coords.push_back(p.second.coordinates);
    }

    projector_ = {stop_coords.begin(), stop_coords.end(), settings_.GetWidth(),
                  settings_.GetHeight(), settings_.GetPadding()};
}

void MapRenderer::AddLinesBetweenStops(svg::Document& doc, std::vector<const Bus*> buses,
                                       const std::unordered_map<std::string_view, Stop>& stops) const {
    assert(!settings_.GetColorPalette().empty());
    int idx_line_color = 0;

    for (const Bus* bus_ptr : buses) {
        svg::Polyline polyline;

        if (bus_ptr->stops.empty()) {
            continue;
        }

        for (std::string_view stop_name : bus_ptr->stops) {
            svg::Point map_stop_point = projector_(stops.at(stop_name).coordinates);
            polyline.AddPoint(map_stop_point);
        }

        polyline.SetStrokeColor(settings_.GetColorPalette()[idx_line_color])
            .SetFillColor(svg::NoneColor)
            .SetStrokeWidth(settings_.GetLineWidth())
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        doc.Add(polyline);

        // another color for next bus route
        idx_line_color = GetNextColorPaletteIdx(idx_line_color);
    }
}

void MapRenderer::AddRouteNames(svg::Document& doc, std::vector<const Bus*> buses,
                                const std::unordered_map<std::string_view, Stop>& stops) const {
    assert(!settings_.GetColorPalette().empty());
    int idx_color_palette = 0;

    for (const Bus* bus_ptr : buses) {
        if (bus_ptr->stops.empty()) {
            continue;
        }

        std::string_view first_stop_name = bus_ptr->stops.front();
        svg::Point first_stop_point = projector_(stops.at(first_stop_name).coordinates);
        AddOneRouteName(doc, bus_ptr->name, first_stop_point, idx_color_palette);

        // one-way route shows the name also at the last one-way stop
        if (!bus_ptr->is_roundtrip &&
            // the first and the last stop can be the same in one-way route
            bus_ptr->stops.front() != bus_ptr->stops[bus_ptr->stops.size() / 2]) {
            std::string_view last_stop_name = bus_ptr->stops[bus_ptr->stops.size() / 2];
            svg::Point last_stop_point = projector_(stops.at(last_stop_name).coordinates);
            AddOneRouteName(doc, bus_ptr->name, last_stop_point, idx_color_palette);
        }

        // another color for next bus route
        idx_color_palette = GetNextColorPaletteIdx(idx_color_palette);
    }
}

void MapRenderer::AddOneRouteName(svg::Document& doc, std::string_view stop_name,
                                  svg::Point& stop_point, int idx_color) const {
    detail::RouteNameUnderlayer name_underlayer(stop_name, stop_point, settings_);
    doc.Add(name_underlayer.GetObject());

    detail::RouteNameText name_text(stop_name, idx_color, stop_point, settings_);
    doc.Add(name_text.GetObject());
}

void MapRenderer::AddStopSymbols(svg::Document& doc, const std::unordered_map<std::string_view, Stop>& stops) const {
    using namespace std::literals;

    // render in ascending order by stop names
    std::vector<std::string_view> sorted_stop_names(stops.size());

    std::transform(
        std::execution::par,
        stops.begin(), stops.end(),
        sorted_stop_names.begin(),
        [](const auto& p) {
            return p.first;
        });
    std::sort(sorted_stop_names.begin(), sorted_stop_names.end());

    // add symbols
    for (std::string_view stop_name : sorted_stop_names) {
        const Stop& stop = stops.at(stop_name);

        if (stop.buses.empty()) {
            continue;
        }

        doc.Add(svg::Circle()
                    .SetCenter(projector_(stop.coordinates))
                    .SetRadius(settings_.GetStopRadius())
                    .SetFillColor("white"s));
    }
}

void MapRenderer::AddStopNames(svg::Document& doc, const std::unordered_map<std::string_view, Stop>& stops) const {
    // render stop names in ascending order
    std::vector<std::string_view> sorted_stop_names(stops.size());

    std::transform(
        std::execution::par,
        stops.begin(), stops.end(),
        sorted_stop_names.begin(),
        [](const auto& p) {
            return p.first;
        });
    std::sort(sorted_stop_names.begin(), sorted_stop_names.end());

    // add stop names
    for (std::string_view stop_name : sorted_stop_names) {
        const Stop& stop = stops.at(stop_name);

        if (stop.buses.empty()) {
            continue;
        }

        svg::Point stop_point = projector_(stops.at(stop_name).coordinates);
        AddOneStopName(doc, stop_name, stop_point);
    }
}

void MapRenderer::AddOneStopName(svg::Document& doc, std::string_view stop_name, svg::Point& stop_point) const {
    detail::StopNameUnderlayer name_underlayer(stop_name, stop_point, settings_);
    doc.Add(name_underlayer.GetObject());

    detail::StopNameText name_text(stop_name, stop_point, settings_);
    doc.Add(name_text.GetObject());
}

}  // namespace renderer
}  // namespace route
