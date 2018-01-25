

set(server_library_dependencies boost_system crypto ssl cpprest ${CMAKE_THREAD_LIBS_INIT} ${PROTOBUF_LIBRARY} jutils)

add_library(server OBJECT server.cc manager.cc metadata.cc session.cc)
add_executable(attguard
  main.cc $<TARGET_OBJECTS:server> $<TARGET_OBJECTS:common> $<TARGET_OBJECTS:proto>)
target_link_libraries(attguard ${server_library_dependencies})

install(TARGETS attguard
  RUNTIME DESTINATION bin)

