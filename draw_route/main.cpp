#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

int main() {
    route::TransportCatalogue catalogue;

    route::request::HandleInput(catalogue);

    route::request::HandleStat(catalogue);

    return 0;
}
