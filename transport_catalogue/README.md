# TransportCatalogue

Works with JSON requests. A route drawing request produces a one-line string answer of SVG format. Also, JSON constructor has been developed with method chaining and a possibility to check for errors during compilation time.

## Consists of
- `json.h` - library for working with JSON
- `json_builder.h` - allows you to create JSON more conveniently:
    ```
    json::Builder{}
        .StartDict()
            .Key("id1").Value("name1")
            .Key("id2").Value("name2")
        .EndDict()
    ```
- `svg.h` - library for working with SVG
- `.proto` files
- other files relevant to `transport_catalogue.h`

## Stack

- C++17
- Protobuf
- CMake
- _custom JSON and SVG libraries_

## How to run

Unix, in bash:
- download [protobuf-cpp](https://github.com/protocolbuffers/protobuf/releases/latest) and unzip it
- in project root type:
    ```
    mkdir build_protobuf
    cd build_protobuf

    cmake <path to downloaded protobuf>/cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -Dprotobuf_BUILD_TESTS=OFF \
        -DCMAKE_INSTALL_PREFIX=../protobuf_package

    cmake --build .

    cmake --install .
    
    // in protobuf_package now we have bin, lib, include directories
    ```
-
    ```
    mkdir ../build_transport_catalogue
    cd ../build_transport_catalogue

    cmake .. -DCMAKE_PREFIX_PATH=protobuf_package
    cmake --build .
    ```
- execute the built file.

## Need to

- rewrite tests for building with CMake
