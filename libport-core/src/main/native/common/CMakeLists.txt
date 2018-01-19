
add_library(common OBJECT utils.cc log.cc configs.cc)
set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
