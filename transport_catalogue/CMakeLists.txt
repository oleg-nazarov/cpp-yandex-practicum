cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
find_package(TBB REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    map_renderer.proto svg.proto transport_catalogue.proto transport_router.proto)

set(TRANSPORT_CATALOGUE_FILES
    main.cpp
    domain.h domain.cpp
    geo.h geo.cpp
    graph.h ranges.h router.h
    json_reader.h json_reader.cpp
    request_handler.h request_handler.cpp
    transport_catalogue.h transport_catalogue.cpp
    transport_router.h transport_router.cpp)
set(JSON_FILES
    json/json.h json/json.cpp
    json/json_builder.h json/json_builder.cpp)
set(SVG_FILES svg/svg.h svg/svg.cpp)
set(MAP_RENDERER_FILES map_renderer.h map_renderer.cpp)
set(SERIALIZATION_FILES serialization.h serialization.cpp)

add_executable(transport_catalogue
    ${PROTO_SRCS} ${PROTO_HDRS}
    ${TRANSPORT_CATALOGUE_FILES}
    ${JSON_FILES} ${SVG_FILES} ${MAP_RENDERER_FILES}
    ${SERIALIZATION_FILES})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue
    "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>"
    Threads::Threads TBB::tbb)
