

include(FindProtobuf)
find_package(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS statement.proto)
ADD_LIBRARY(proto SHARED ${PROTO_HDRS} ${PROTO_SRCS})
