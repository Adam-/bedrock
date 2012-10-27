
include_directories(../../include ${ZLIB_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${EVENT_INCLUDE_DIR})
add_library(util SHARED buffer.c compression.c crypto.c endian.c fd.c file.c io.c list.c memory.c pipe.c string.c thread.c)
target_link_libraries(util ${ZLIB_LIBRARIES} ${OPENSSL_LIBRARIES} ${EVENT_LIBRARY})
if(WIN32)
  target_link_libraries(util ${PTHREAD_LIBRARY})
else(WIN32)
  target_link_libraries(util pthread)
endif(WIN32)

