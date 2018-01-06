


protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS statement.proto)
ADD_LIBRARY(proto STATIC ${PROTO_HDRS} ${PROTO_SRCS})
set_property(TARGET proto PROPERTY POSITION_INDEPENDENT_CODE ON)
