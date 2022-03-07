# Spreadsheet

Allows storing data in tabular form. Each cell may contain text or numeric data, or the result of formula calculation (formula can refer to another cell). There is also possible to check the formula for correctness and find a circular dependency.

#

## Details
- `Cell` class contains a structure that represents bidirected graph:
  - for resetting the cache of dependent cells
  - for being sensitive to the change of referenced cells
- `Formula` has a cache for computation optimization

#

## Tech stack

- C++17
- CMake
- [antlr4](https://www.antlr.org/)

#

## How to run

- install Java SE Runtime Environment 8  
- install [ANTLR 4](https://www.antlr.org/)  
  - we need antlr4 и grun  
  - check that CLASSPATH contains JAR-file antlr*.jar
  - put C++ Target (look for it on the link above) inside this directory and name it "antlr4_runtime" (because we use that name in CMakeLists.txt)
- create _build_ folder, go there and call `cmake ..` and `cmake --build .`

#

## Future plans

- optimize the cells' storage (research for sparse matrix, trees, something else)
- Add a new error type for `FormulaError::Category` (`#NUM!`) that will be caused by too big a number (currently it shows `#DIV/0!`)
  