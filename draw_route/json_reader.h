#pragma once

#include <iostream>

#include "transport_catalogue.h"

namespace route {
namespace io {

void ReadJSON(std::istream& input, std::ostream& output, TransportCatalogue& catalogue);

}  // namespace io
}  // namespace route
