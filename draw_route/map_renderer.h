#pragma once

#include <algorithm>
#include <execution>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "domain.h"
#include "geo.h"
#include "svg/svg.h"

namespace route {
namespace renderer {

class MapSettings;

namespace detail {

// ---------- RouteName ----------

class RouteName {
   public:
    RouteName(std::string_view name, const svg::Point& point, const MapSettings& settings);

    virtual svg::Text GetObject() const;

   protected:
    const std::string font_family_ = "Verdana";
    const std::string font_weight_ = "bold";
    std::string_view name_;
    svg::Point point_;
    const MapSettings& settings_;
};

class RouteNameText : public RouteName {
   public:
    RouteNameText(std::string_view name, int idx_color_palete, const svg::Point& point, const MapSettings& settings);

    svg::Text GetObject() const override;

   private:
    svg::Color fill_color_;
};

class RouteNameUnderlayer : public RouteName {
   public:
    RouteNameUnderlayer(std::string_view name, const svg::Point& point, const MapSettings& settings);

    svg::Text GetObject() const override;

   private:
    svg::Color fill_color_;
    svg::Color stroke_color_;
    double stroke_width_;
    svg::StrokeLineCap stroke_line_cap_ = svg::StrokeLineCap::ROUND;
    svg::StrokeLineJoin stroke_line_join_ = svg::StrokeLineJoin::ROUND;
};

// ---------- StopName ----------

class StopName {
   public:
    StopName(std::string_view name, const svg::Point& point, const MapSettings& settings);

    virtual svg::Text GetObject() const;

   protected:
    const std::string font_family_ = "Verdana";
    std::string_view name_;
    svg::Point point_;
    const MapSettings& settings_;
};

class StopNameText : public StopName {
   public:
    StopNameText(std::string_view name, const svg::Point& point, const MapSettings& settings);

    svg::Text GetObject() const override;

   private:
    svg::Color fill_color_ = "black";
};

class StopNameUnderlayer : public StopName {
   public:
    StopNameUnderlayer(std::string_view name, const svg::Point& point, const MapSettings& settings);

    svg::Text GetObject() const override;

   private:
    svg::Color fill_color_;
    svg::Color stroke_color_;
    double stroke_width_;
    svg::StrokeLineCap stroke_line_cap_ = svg::StrokeLineCap::ROUND;
    svg::StrokeLineJoin stroke_line_join_ = svg::StrokeLineJoin::ROUND;
};

// ---------- SphereProjector ----------

inline const double EPSILON = 1e-6;
inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
   public:
    SphereProjector() = default;
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
                    double max_height, double padding)
        : padding_(padding) {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            std::execution::par,
            points_begin, points_end,
            [](const auto& lhs, const auto& rhs) {
                return lhs.lng < rhs.lng;
            });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            std::execution::par,
            points_begin, points_end,
            [](const auto& lhs, const auto& rhs) {
                return lhs.lat < rhs.lat;
            });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
    }

   private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

}  // namespace detail

class MapSettings {
   public:
    void SetWidth(double width);
    double GetWidth() const noexcept;

    void SetHeight(double height);
    double GetHeight() const noexcept;

    void SetPadding(double padding);
    double GetPadding() const noexcept;

    void SetLineWidth(double line_width);
    double GetLineWidth() const noexcept;

    void SetStopRadius(double stop_radius);
    double GetStopRadius() const noexcept;

    void SetBusLabelFontSize(int bus_label_font_size);
    int GetBusLabelFontSize() const noexcept;

    void SetBusLabelOffset(std::vector<double>&& bus_label_offset);
    std::vector<double> GetBusLabelOffset() const noexcept;

    void SetStopLabelFontSize(int stop_label_font_size);
    int GetStopLabelFontSize() const noexcept;

    void SetStopLabelOffset(std::vector<double>&& stop_label_offset);
    std::vector<double> GetStopLabelOffset() const noexcept;

    void SetUnderlayerColor(svg::Color&& underlayer_color);
    svg::Color GetUnderlayerColor() const noexcept;

    void SetUnderlayerWidth(double underlayer_width);
    double GetUnderlayerWidth() const noexcept;

    void SetColorPalette(std::vector<svg::Color>&& color_palette);
    std::vector<svg::Color> GetColorPalette() const noexcept;

   private:
    double width_;
    double height_;
    double padding_;
    double line_width_;
    double stop_radius_;
    int bus_label_font_size_;
    std::vector<double> bus_label_offset_;
    int stop_label_font_size_;
    std::vector<double> stop_label_offset_;
    svg::Color underlayer_color_;
    double underlayer_width_;
    std::vector<svg::Color> color_palette_;
};

class MapRenderer {
   public:
    MapRenderer(MapSettings&& settings);

    std::string GetMapSVG(std::vector<const Bus*> buses,
                          const std::unordered_map<std::string_view, Stop>& stops);

   private:
    detail::SphereProjector projector_;
    const MapSettings settings_;

    int GetNextColorPaletteIdx(int cuurent_idx) const noexcept;

    void SetUpProjector(const std::unordered_map<std::string_view, Stop>& stops);

    void AddLinesBetweenStops(svg::Document& doc, std::vector<const Bus*> buses,
                              const std::unordered_map<std::string_view, Stop>& stops) const;
    void AddRouteNames(svg::Document& doc, std::vector<const Bus*> buses,
                       const std::unordered_map<std::string_view, Stop>& stops) const;
    void AddOneRouteName(svg::Document& doc, std::string_view stop_name, svg::Point& stop_point,
                         int idx_color) const;
    void AddStopSymbols(svg::Document& doc, const std::unordered_map<std::string_view, Stop>& stops) const;
    void AddStopNames(svg::Document& doc, const std::unordered_map<std::string_view, Stop>& stops) const;
    void AddOneStopName(svg::Document& doc, std::string_view stop_name, svg::Point& stop_point) const;
};

}  // namespace renderer
}  // namespace route
