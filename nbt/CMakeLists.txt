
include_directories(../include ${ZLIB_INCLUDE_DIRS})
add_library(nbt STATIC nbt.c yml.c)
target_link_libraries(nbt util)

add_executable(editor editor.c dumper.c)
target_link_libraries(editor nbt util)
if(WIN32)
  target_link_libraries(editor windows)
endif(WIN32)
set_target_properties(editor PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON BUILD_WITH_INSTALL_RPATH ON)

install(TARGETS nbt DESTINATION lib)
install(TARGETS editor DESTINATION bin)
