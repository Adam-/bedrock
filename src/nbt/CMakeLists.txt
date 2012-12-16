
include_directories(../../include ${ZLIB_INCLUDE_DIRS})
add_library(nbt STATIC nbt.c)
target_link_libraries(nbt util)

add_executable(editor editor.c dumper.c)
target_link_libraries(editor nbt util)
if(WIN32)
  target_link_libraries(editor windows)
endif(WIN32)
