cmake_minimum_required(VERSION 2.8)

project(bedrock C)

if(WIN32)
  add_definitions(-DWIN32)
endif(WIN32)

# include cmake stuff for checking various system specific defines
include(CheckTypeSize)

# Check type sizes since we need them
check_type_size(int8_t INT8_T)
check_type_size(uint8_t UINT8_T)
check_type_size(int16_t INT16_T)
check_type_size(uint16_t UINT16_T)
check_type_size(int32_t INT32_T)
check_type_size(uint32_t UINT32_T)
check_type_size(int64_t INT64_T)
check_type_size(uint64_t UINT64_T)

set(CMAKE_MODULE_PATH cmake)

# required packages for compile
find_package(ZLIB REQUIRED)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(libevent REQUIRED)
if(WIN32)
  find_package(libpthread REQUIRED)
endif(WIN32)
if(NOT WIN32)
  find_package(Curses)
endif(NOT WIN32)

set(CMAKE_C_FLAGS "-g -pipe -Wall -Wextra -std=gnu99 -pedantic")

add_subdirectory(src)

