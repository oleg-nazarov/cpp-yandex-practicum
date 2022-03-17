# Mython

Interpretator for Mython language.

## Details

- lexical parser (`lexer.h`) parses a source code into tokens
- `parse.h` creates Abstract Syntax Tree
- runtime module (`runtime.h`)

## How to run tests

```
g++ -std=c++17 *.cpp
./a.out
```

## Need to

- separate tests from `main.cpp`
