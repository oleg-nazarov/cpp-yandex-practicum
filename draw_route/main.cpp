#include <iostream>

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

int main() {
    route::TransportCatalogue catalogue;

    route::input_request::Read(std::cin, catalogue);

    route::stat_request::Read(std::cin, std::cout, catalogue);

    return 0;
}
