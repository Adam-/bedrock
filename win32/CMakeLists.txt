
#include_directories(../../include ${ZLIB_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${EVENT_INCLUDE_DIR})
include_directories(../include)
add_library(windows STATIC atomic.c getopt.c pipe.c pthread.c windows_time.c)# arpa/inet.c)
target_link_libraries(windows ws2_32 ${EVENT_LIBRARY})

