
# Include library header files
include_directories(../include ../include/server ${OPENSSL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})

# Set an empty list for later
set(BEDROCK_SOURCES)

# Get a list of folders from src/ and remove any CMake generated build files
file(GLOB SOURCE_FOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")

# Get the source files and append them to a list
foreach(SOURCE_FOLDER ${SOURCE_FOLDERS})
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FOLDER}")
    file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${SOURCE_FOLDER}/*.c")
    list(APPEND BEDROCK_SOURCES ${SOURCE_FILES})
  endif(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FOLDER}")
endforeach(SOURCE_FOLDER ${SOURCE_FOLDERS})

# Sort the sources
list(SORT BEDROCK_SOURCES)

if(WIN32)
  file(GLOB SOURCE_FOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "../win32/*")

  foreach(SOURCE_FOLDER ${SOURCE_FOLDERS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FOLDER}")
      file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${SOURCE_FOLDER}/*.c")
      list(APPEND BEDROCK_SOURCES ${SOURCE_FILES})
    else(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FOLDER}")
      list(APPEND BEDROCK_SOURCES ${SOURCE_FOLDER})
    endif(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FOLDER}")
  endforeach(SOURCE_FOLDER ${SOURCE_FOLDERS})
endif(WIN32)

add_executable(bedrock ${BEDROCK_SOURCES})
target_link_libraries(bedrock nbt util)
if(WIN32)
  target_link_libraries(bedrock ws2_32)
else(WIN32)
  target_link_libraries(bedrock m rt)
endif(WIN32)
set_target_properties(bedrock PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON BUILD_WITH_INSTALL_RPATH ON)

install(TARGETS bedrock DESTINATION bin)

