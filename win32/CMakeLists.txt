
include_directories(../include)
add_library(windows STATIC getopt.c pipe.c pthread.c windows_time.c)
target_link_libraries(windows ws2_32 ${EVENT_LIBRARY})

