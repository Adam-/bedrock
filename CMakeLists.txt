cmake_minimum_required(VERSION 2.8)

include_directories(include)

# Get a list of folders from src/ and remove any CMake crap
file(GLOB SOURCE_FOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*")
list(REMOVE_ITEM SOURCE_FOLDERS "src/CMakeFiles")

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

add_executable(bedrock ${BEDROCK_SOURCES})
target_link_libraries(bedrock z m rt pthread)

