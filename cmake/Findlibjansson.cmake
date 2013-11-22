
find_path(JANSSON_INCLUDE_DIR jansson.h
  PATHS /usr/include
)

find_library(JANSSON_LIBRARY
  NAMES jansson
  PATHS /usr/lib /usr/local/lib
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(JANSSON
  DEFAULT_MSG
  JANSSON_INCLUDE_DIR
  JANSSON_LIBRARY
)

include_directories(${JANSSON_INCLUDE_DIR})
