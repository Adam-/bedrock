
include_directories(../../include)
add_library(nbt SHARED nbt.c)

add_executable(editor editor.c dumper.c)
target_link_libraries(editor nbt util)
