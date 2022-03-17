# My C++ projects after "Practicum by Yandex" bootcamp

## [Spreadsheet](spreadsheet/)

Allows storing data in tabular form. Each cell may contain text or numeric data, or the result of formula calculation (formula can refer to another cell). There is also possible to check the formula for the correctness and find a circular dependency.  
stack: C++17, CMake, antlr4.

## [TransportCatalogue](transport_catalogue/)

Works with JSON requests. A route drawing request produces a one-line string answer of SVG format. Also, JSON constructor has been developed with method chaining and a possibility to check for errors during compilation time.  
stack: C++17, Protobuf, CMake, custom JSON and SVG libraries

## [SearchEngine](search_engine/)

This search engine application is able to find a certain document across all added documents. It is possible to use minus words to exclude some documents from the result. The order of the result is based on the TF-IDF priority rank system.  

## [Vector](vector/)

A container that is similar to `std::vector`. Provides strong exception safety, uses RAII, and placement new operator. This container is efficient and its number of called constructors, assignments, and destructors are the same as of `std::vector` (see _Benchmark_ test)  

## [SinglyLinkedList](singly_linked_list/)

Singly-linked list. It uses the written custom Forward Iterator, provides strong exception safety, and has unit tests.  
