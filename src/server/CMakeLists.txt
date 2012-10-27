
# Include library header files
include_directories(../../include ../../include/server)
if(WIN32)
  include_directories(win32 ${PTHREAD_INCLUDE_DIR})
endif(WIN32)

# Generate the files for compile
FLEX_TARGET(LEXER ${CMAKE_CURRENT_SOURCE_DIR}/config/lexer.l ${CMAKE_CURRENT_SOURCE_DIR}/config/lexer.c)
BISON_TARGET(PARSER ${CMAKE_CURRENT_SOURCE_DIR}/config/parser.y ${CMAKE_CURRENT_SOURCE_DIR}/config/parser.c)
ADD_FLEX_BISON_DEPENDENCY(LEXER PARSER)

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

add_executable(bedrock ${BEDROCK_SOURCES} ${FLEX_LEXER_OUTPUTS} ${BISON_PARSER_OUTPUTS})
target_link_libraries(bedrock nbt util)
if(WIN32)
  target_link_libraries(bedrock ws2_32)
else(WIN32)
  target_link_libraries(bedrock m rt)
endif(WIN32)

