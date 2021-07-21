#include "json_reader.h"

#include <vector>

#include "domain.h"
#include "json/json.h"
#include "map_renderer.h"
#include "request_handler.h"

namespace route {
namespace io {

void HandleRequests(const json::Node& json_node, const std::vector<const RequestHandler*>& handlers) {
    const json::Dict& json_map = json_node.AsMap();

    for (const RequestHandler* handler : handlers) {
        const json::Node& request_node = json_map.at(handler->GetRequestType());
        handler->Handle(request_node);
    }
}

void ReadJSON(std::istream& input, std::ostream& output, TransportCatalogue& catalogue) {
    const json::Document document = json::Load(input);
    const json::Node& json_node = document.GetRoot();

    // handle map settings before creating map_renderer
    renderer::MapSettings map_settings;
    SetMapSettingsHandler settings_h(map_settings);
    HandleRequests(json_node, {&settings_h});

    // handle other requests
    route::renderer::MapRenderer map_renderer(std::move(map_settings));
    AddDataHandler add_h(catalogue);
    GetDataHandler get_h(output, catalogue, map_renderer);

    // order is important: firstlly - add data, only then - get new information
    std::vector<const RequestHandler*> handlers{&add_h, &get_h};
    HandleRequests(json_node, handlers);
}

}  // namespace io
}  // namespace route