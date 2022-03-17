# Vector

A container that is similar to `std::vector`. Provides strong exception safety, uses RAII, and placement new operator. This container is efficient and its number of called constructors, assignments, and destructors are the same as of `std::vector` (see _Benchmark_ test)

## Consists of

- `class RawMemory` handles allocation and deallocation of memory without initializing objects.
- `class Vector`

## How to run tests

```
g++ __tests__/vector__test.cpp
./a.out
```
