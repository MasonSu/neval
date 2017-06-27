Neval
=====
A lightweight but efficient http server

## Features
* support select and epoll
* timer(binary heap)
* support configure file
* support HTTP keep-alive
* support HTTP cache-control
* tested with Valgrind

## Build and Run
please make sure you have [cmake](https://cmake.org/) installed
```
mkdir build && cd build
cmake ..
make && make install
cd .. && ./neval
```
