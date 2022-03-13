#pragma once

#include <iostream>

#include "transport_catalogue.h"

namespace route {
namespace io {

void ReadJSON(std::istream& input, std::ostream& output, TransportCatalogue& catalogue);

void ReadMakeBaseJSON(std::istream& input);
void ReadProcessRequestsJSON(std::istream& input, std::ostream& output);

}  // namespace io
}  // namespace route
