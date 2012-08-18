cmake_minimum_required(VERSION 2.8)

project(bedrock C)

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

# required packages for compile
find_package(ZLIB REQUIRED)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Curses)

# Include library header files
include_directories(include ${ZLIB_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

# Generate the files for compile
FLEX_TARGET(LEXER ${CMAKE_CURRENT_SOURCE_DIR}/src/config/lexer.l
${CMAKE_CURRENT_SOURCE_DIR}/src/config/lexer.c)
BISON_TARGET(PARSER ${CMAKE_CURRENT_SOURCE_DIR}/src/config/parser.y
${CMAKE_CURRENT_SOURCE_DIR}/src/config/parser.c)
ADD_FLEX_BISON_DEPENDENCY(LEXER PARSER)

# Get a list of folders from src/ and remove any CMake generated build files
file(GLOB SOURCE_FOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*")
list(REMOVE_ITEM SOURCE_FOLDERS "src/CMakeFiles")
# The console is its own separate program
list(REMOVE_ITEM SOURCE_FOLDERS "src/console")

# Set an empty list for later
set(BEDROCK_SOURCES)

# Get the source files and append them to a list
foreach(SOURCE_FOLDER ${SOURCE_FOLDERS})
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FOLDER}")
    file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${SOURCE_FOLDER}/*.c")
    list(APPEND BEDROCK_SOURCES ${SOURCE_FILES})
  endif(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FOLDER}")
endforeach(SOURCE_FOLDER ${SOURCE_FOLDERS})

# Sort the sources
list(SORT BEDROCK_SOURCES)

set(CMAKE_C_FLAGS "-g -pipe -Wall -Wextra -std=gnu99 -pedantic")

add_executable(bedrock ${BEDROCK_SOURCES} ${FLEX_LEXER_OUTPUTS} ${BISON_PARSER_OUTPUTS})
target_link_libraries(bedrock m rt pthread ${ZLIB_LIBRARIES} ${OPENSSL_LIBRARIES})

if(CURSES_FOUND)
  file(GLOB CONSOLE_SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/console/*.c")
  add_executable(console ${CONSOLE_SOURCE_FILES})
  target_link_libraries(console ${CURSES_LIBRARIES})
endif(CURSES_FOUND)

