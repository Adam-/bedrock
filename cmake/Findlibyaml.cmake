
find_path(YAML_INCLUDE_DIR yaml.h
  PATHS /usr/include
)

find_library(YAML_LIBRARY
  NAMES yaml
  PATHS /usr/lib /usr/local/lib
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(YAML
  DEFAULT_MSG
  YAML_INCLUDE_DIR
  YAML_LIBRARY
)

include_directories(${YAML_INCLUDE_DIR})
