
include_directories(../../include ../../include/server)

if(NOT WIN32 AND CURSES_FOUND)
  file(GLOB CONSOLE_SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.c")
  add_executable(console ${CONSOLE_SOURCE_FILES})
  target_link_libraries(console ${CURSES_LIBRARIES} ${EVENT_LIBRARY})
endif(NOT WIN32 AND CURSES_FOUND)

