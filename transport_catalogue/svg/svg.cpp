#include "svg.h"

#include <iterator>
#include <memory>
#include <string>
#include <string_view>

namespace svg {

using namespace std::literals;

// ---------- Color ----------

Rgb::Rgb() : red(0), green(0), blue(0) {}
Rgb::Rgb(uint8_t red, uint8_t green, uint8_t blue) : red(red), green(green), blue(blue) {}

Rgba::Rgba() : Rgb(), opacity(1.0) {}
Rgba::Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity) : Rgb(red, green, blue), opacity(opacity) {}

// ---------- PathProps ----------

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap) {
    using namespace std::literals;

    switch (line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
            break;
        default:
            break;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join) {
    using namespace std::literals;

    switch (line_join) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
        default:
            break;
    }

    return out;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ----------

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;

    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Polyline ----------

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(std::move(point));
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;

    out << "<polyline points=\""sv;
    for (size_t i = 0; i < points_.size(); ++i) {
        out << points_[i].x << ',' << points_[i].y;

        if (i + 1 < points_.size()) {
            out << ' ';
        }
    }
    out << "\"";

    RenderAttrs(out);

    out << "/>"sv;
}

// ---------- Text ----------

Text& Text::SetPosition(Point pos) {
    position_ = std::move(pos);
    return *this;
}
Text& Text::SetOffset(Point offset) {
    offset_ = std::move(offset);
    return *this;
}
Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}
Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}
Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}
Text& Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;

    out << "<text"sv;

    RenderAttrs(out);

    out << " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\""sv;
    out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
    out << " font-size=\""sv << font_size_ << "\""sv;

    if (font_family_) {
        out << " font-family=\""sv << *font_family_ << "\""sv;
    }
    if (font_weight_) {
        out << " font-weight=\""sv << *font_weight_ << "\""sv;
    }

    out << ">"sv;

    out << ProcessData(data_) << "</text>"sv;
}

std::string Text::ProcessData(std::string_view data_sv) const {
    std::string processed_data;

    while (true) {
        size_t pos = data_sv.find_first_of("\"'<>&");

        if (pos == data_sv.npos) {
            processed_data += std::string(data_sv);
            break;
        }

        processed_data += std::string(data_sv.substr(0, pos));

        switch (data_sv[pos]) {
            case '"':
                processed_data += "&quot;";
                break;

            case '\'':
                processed_data += "&apos;";
                break;

            case '<':
                processed_data += "&lt;";
                break;

            case '>':
                processed_data += "&gt;";
                break;

            case '&':
                processed_data += "&amp;";
                break;

            default:
                break;
        }

        data_sv.remove_prefix(pos + 1);
    }

    return processed_data;
}

// ---------- Document ----------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_ptr_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

    RenderContext ctx(out, 0, 2);
    for (const auto& ptr : objects_ptr_) {
        ptr->Render(ctx);
    }

    out << "</svg>"sv << std::endl;
}

}  // namespace svg
