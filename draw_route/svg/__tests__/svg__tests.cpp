#define _USE_MATH_DEFINES
#include <cmath>
#include <iterator>
#include <sstream>
#include <string>

#include "../../../helpers/run_test.h"
#include "../svg.h"

using namespace std::literals;
using namespace svg;

Polyline CreateStar(Point center, double outer_rad, double inner_rad, int num_rays) {
    Polyline polyline;

    for (int i = 0; i <= num_rays; ++i) {
        double angle = 2 * M_PI * (i % num_rays) / num_rays;
        polyline.AddPoint({center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle)});
        if (i == num_rays) {
            break;
        }
        angle += M_PI / num_rays;
        polyline.AddPoint({center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle)});
    }

    return polyline;
}

namespace shapes {

class Triangle : public Drawable {
   public:
    Triangle(Point p1, Point p2, Point p3) : p1_(std::move(p1)), p2_(std::move(p2)), p3_(std::move(p3)) {}

    void Draw(ObjectContainer& container) const override {
        container.Add(Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
    }

   private:
    Point p1_, p2_, p3_;
};

class Star : public Drawable {
   public:
    Star(Point center, double outer_rad, double inner_rad, int num_rays)
        : center_(std::move(center)), outer_rad_(outer_rad), inner_rad_(inner_rad), num_rays_(num_rays) {}

    void Draw(ObjectContainer& container) const override {
        container.Add(CreateStar(center_, outer_rad_, inner_rad_, num_rays_)
                          .SetFillColor("red"s)
                          .SetStrokeColor("black"s));
    }

   private:
    Point center_;
    double outer_rad_;
    double inner_rad_;
    int num_rays_;
};

class Snowman : public Drawable {
   public:
    Snowman(Point head_center, double head_radius)
        : head_center_(std::move(head_center)), head_radius_(head_radius) {}

    void Draw(ObjectContainer& container) const override {
        using namespace std::literals;

        // bottom circle
        container.Add(Circle()
                          .SetCenter({head_center_.x, head_center_.y + head_radius_ * 5})
                          .SetRadius(head_radius_ * 2)
                          .SetFillColor("rgb(240,240,240)"s)
                          .SetStrokeColor("black"s));

        // middle circle
        container.Add(Circle()
                          .SetCenter({head_center_.x, head_center_.y + head_radius_ * 2})
                          .SetRadius(head_radius_ * 1.5)
                          .SetFillColor("rgb(240,240,240)"s)
                          .SetStrokeColor("black"s));

        // top circle
        container.Add(Circle()
                          .SetCenter({head_center_.x, head_center_.y})
                          .SetRadius(head_radius_)
                          .SetFillColor("rgb(240,240,240)"s)
                          .SetStrokeColor("black"s));
    }

   private:
    Point head_center_;
    double head_radius_;
};

}  // namespace shapes

template <typename DrawableIterator>
void DrawPicture(DrawableIterator begin, DrawableIterator end, svg::ObjectContainer& target) {
    for (auto it = begin; it != end; ++it) {
        (*it)->Draw(target);
    }
}

template <typename Container>
void DrawPicture(const Container& container, svg::ObjectContainer& target) {
    using namespace std;
    DrawPicture(begin(container), end(container), target);
}

void TestMockedSnowmanStarTriangle() {
    using namespace svg;
    using namespace shapes;
    using namespace std;

    std::ostringstream output;
    std::string mocked_output(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"
        "  <polyline points=\"100,20 120,50 80,40 100,20\"/>\n"
        "  <polyline points=\"50,10 52.3511,16.7639 59.5106,16.9098 53.8042,21.2361 55.8779,28.0902 50,24 44.1221,28.0902 46.1958,21.2361 40.4894,16.9098 47.6489,16.7639 50,10\" fill=\"red\" stroke=\"black\"/>\n"
        "  <circle cx=\"30\" cy=\"70\" r=\"20\" fill=\"rgb(240,240,240)\" stroke=\"black\"/>\n"
        "  <circle cx=\"30\" cy=\"40\" r=\"15\" fill=\"rgb(240,240,240)\" stroke=\"black\"/>\n"
        "  <circle cx=\"30\" cy=\"20\" r=\"10\" fill=\"rgb(240,240,240)\" stroke=\"black\"/>\n"
        "  <text x=\"10\" y=\"100\" dx=\"0\" dy=\"0\" font-size=\"12\" font-family=\"Verdana\" fill=\"yellow\" stroke=\"yellow\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">Happy New Year!</text>\n"
        "  <text x=\"10\" y=\"100\" dx=\"0\" dy=\"0\" font-size=\"12\" font-family=\"Verdana\" fill=\"red\">Happy New Year!</text>\n"
        "</svg>\n");

    vector<unique_ptr<svg::Drawable>> picture;
    picture.emplace_back(make_unique<Triangle>(Point{100, 20}, Point{120, 50}, Point{80, 40}));
    picture.emplace_back(make_unique<Star>(Point{50.0, 20.0}, 10.0, 4.0, 5));
    picture.emplace_back(make_unique<Snowman>(Point{30, 20}, 10.0));

    svg::Document doc;
    DrawPicture(picture, doc);

    const Text base_text = Text()
                               .SetFontFamily("Verdana"s)
                               .SetFontSize(12)
                               .SetPosition({10, 100})
                               .SetData("Happy New Year!"s);
    doc.Add(Text{base_text}
                .SetStrokeColor("yellow"s)
                .SetFillColor("yellow"s)
                .SetStrokeLineJoin(StrokeLineJoin::ROUND)
                .SetStrokeLineCap(StrokeLineCap::ROUND)
                .SetStrokeWidth(3));
    doc.Add(Text{base_text}.SetFillColor("red"s));

    doc.Render(output);

    ASSERT_EQUAL(output.str(), mocked_output);
}

void TestRgb() {
    svg::Rgb rgb{255, 0, 100};
    ASSERT(rgb.red == 255);
    ASSERT(rgb.green == 0);
    ASSERT(rgb.blue == 100);

    svg::Rgb color;

    ASSERT(color.red == 0 && color.green == 0 && color.blue == 0);
}

void TestRgba() {
    svg::Rgba rgba{100, 20, 50, 0.3};
    ASSERT(rgba.red == 100);
    ASSERT(rgba.green == 20);
    ASSERT(rgba.blue == 50);
    ASSERT(rgba.opacity == 0.3);

    svg::Rgba color;

    ASSERT(color.red == 0 && color.green == 0 && color.blue == 0 && color.opacity == 1.0);
}

int main() {
    RUN_TEST(TestMockedSnowmanStarTriangle);
    RUN_TEST(TestRgb);
    RUN_TEST(TestRgba);

    return 0;
}
