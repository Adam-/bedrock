
find_path(EVENT_INCLUDE_DIR event2/event.h
  PATHS /usr/include
)

find_library(EVENT_LIBRARY
  NAMES event
  PATHS /usr/lib /usr/local/lib
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(EVENT
  DEFAULT_MSG
  EVENT_INCLUDE_DIR
  EVENT_LIBRARY
)

include_directories(${EVENT_INCLUDE_DIR})