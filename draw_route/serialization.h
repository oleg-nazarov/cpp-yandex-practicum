#pragma once

#include <filesystem>

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace route {
namespace io {

struct SerializationSettings {
    std::filesystem::path db_path;
};

void SerializeTCatalogue(const TransportCatalogue& catalogue, const RoutingSettings& routing_settings,
                         const renderer::MapSettings& map_settings, const SerializationSettings& serialization_settings);
void DeserializeTCatalogue(TransportCatalogue& catalogue, RoutingSettings& routing_settings,
                           renderer::MapSettings& map_settings, const SerializationSettings& serialization_settings);

}  // namespace io
}  // namespace route
