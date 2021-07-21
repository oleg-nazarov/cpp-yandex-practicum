#include <iostream>

#include "json_reader.h"
#include "transport_catalogue.h"

int main() {
    route::TransportCatalogue catalogue;

    route::io::ReadJSON(std::cin, std::cout, catalogue);

    return 0;
}
