#pragma once

namespace route {
namespace geo {

struct Coordinates {
    double lat;
    double lng;
};

double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo
}  // namespace route
