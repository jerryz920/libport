
add_library(common STATIC utils.cc log.cc)
set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
