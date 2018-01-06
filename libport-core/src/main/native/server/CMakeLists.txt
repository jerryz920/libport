

set(server_library_dependencies boost_system crypto ssl cpprest ${CMAKE_THREAD_LIBS_INIT} proto ${PROTOBUF_LIBRARY} common)

add_executable(attguard 
  main.cc handlers.cc manager.cc metadata.cc session.cc)
target_link_libraries(attguard ${server_library_dependencies})


