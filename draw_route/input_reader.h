#pragma once

#include "request_processing.h"
#include "transport_catalogue.h"

namespace route {
namespace request {

void HandleInput(TransportCatalogue& catalogue);

void HandleAddStop(TransportCatalogue& catalogue, const Request& request);

void HandleAddBus(TransportCatalogue& catalogue, const Request& request);

}  // namespace request
}  // namespace route
