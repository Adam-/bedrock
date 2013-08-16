
include_directories(../include ${CMAKE_CURRENT_BINARY_DIR}/../include/util/ ${ZLIB_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${EVENT_INCLUDE_DIR})
add_library(util STATIC buffer.c compression.c crypto.c endian.c fd.c file.c io.c list.c memory.c pipe.c string.c thread.c util.c yml.c)
target_link_libraries(util ${ZLIB_LIBRARIES} ${OPENSSL_LIBRARIES} ${EVENT_LIBRARY} ${YAML_LIBRARY})
if(WIN32)
  target_link_libraries(util windows msvcrt)
else(WIN32)
  target_link_libraries(util pthread)
endif(WIN32)

install(TARGETS util DESTINATION lib)

