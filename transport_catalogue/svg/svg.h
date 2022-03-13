#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace svg {

struct Point {
    Point() = default;
    Point(double x, double y) : x(x), y(y) {}

    double x = 0;
    double y = 0;
};

struct RenderContext {
    RenderContext(std::ostream& out) : out(out) {}

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out), indent_step(indent_step), indent(indent) {}

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

// ---------- Color ----------

struct Rgb {
    Rgb();
    Rgb(uint8_t red, uint8_t green, uint8_t blue);

    uint8_t red, green, blue;
};

struct Rgba : public Rgb {
    Rgba();
    Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity);

    double opacity;
};

const std::string NoneColor{"none"};
using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

struct ColorPrinter {
    std::string operator()(std::monostate) const noexcept {
        return NoneColor;
    }

    std::string operator()(std::string color) const noexcept {
        return color;
    }

    std::string operator()(Rgb rgb) const noexcept {
        using namespace std::literals;

        std::ostringstream outs;
        outs << "rgb("s << std::to_string(rgb.red) << ","s << std::to_string(rgb.green) << ","s;
        outs << std::to_string(rgb.blue) << ")"s;

        return outs.str();
    }

    std::string operator()(Rgba rgba) const noexcept {
        using namespace std::literals;
        std::ostringstream outs;

        outs << "rgba("s << std::to_string(rgba.red) << ","s << std::to_string(rgba.green) << ","s;
        outs << std::to_string(rgba.blue) << ","s << rgba.opacity << ")"s;

        return outs.str();
    }
};

// ---------- PathProps ----------

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};
std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap);

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};
std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join);

template <typename Owner>
class PathProps {
   public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }
    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_line_cap_ = std::move(line_cap);
        return AsOwner();
    }
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_line_join_ = std::move(line_join);
        return AsOwner();
    }

   protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv << std::visit(ColorPrinter{}, *fill_color_) << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << std::visit(ColorPrinter{}, *stroke_color_) << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
        }
    }

   private:
    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;

    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }
};

// ---------- Object ----------

class Object {
   public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

   private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

// https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
class Circle final : public Object, public PathProps<Circle> {
   public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

   private:
    Point center_;
    double radius_ = 1.0;

    void RenderObject(const RenderContext& context) const override;
};

// https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
class Polyline final : public Object, public PathProps<Polyline> {
   public:
    Polyline& AddPoint(Point point);

   private:
    std::vector<Point> points_;

    void RenderObject(const RenderContext& context) const override;
};

// https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
class Text final : public Object, public PathProps<Text> {
   public:
    // x, y
    Text& SetPosition(Point pos);

    // dx, dy
    Text& SetOffset(Point offset);

    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);

    Text& SetData(std::string data_);

   private:
    Point position_;
    Point offset_;
    uint32_t font_size_ = 1;
    std::optional<std::string> font_family_;
    std::optional<std::string> font_weight_;
    std::string data_;

    void RenderObject(const RenderContext& context) const override;

    std::string ProcessData(std::string_view data_sv) const;
};

class ObjectContainer {
   public:
    template <typename Obj>
    void Add(Obj obj);

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

   protected:
    ~ObjectContainer() = default;

    std::vector<std::unique_ptr<Object>> objects_ptr_;
};

template <typename Obj>
void ObjectContainer::Add(Obj obj) {
    objects_ptr_.push_back(std::make_unique<Obj>(std::move(obj)));
}

class Drawable {
   public:
    virtual ~Drawable() = default;

    virtual void Draw(ObjectContainer& container) const = 0;
};

class Document final : public ObjectContainer {
   public:
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    void Render(std::ostream& out) const;
};

}  // namespace svg
