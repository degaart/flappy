cmake_minimum_required(VERSION 3.24..3.31)
project(assetPack)

# use current timestamp when extracting archives with ExternalProject or FetchContent
cmake_policy(SET CMP0135 NEW)

# Set C++17 as default
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(assetPack assetPack.cpp)

