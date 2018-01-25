
set(test_library_dependencies boost_system crypto ssl cpprest ${CMAKE_THREAD_LIBS_INIT} ${PROTOBUF_LIBRARY} jutils latte)

file(GLOB testfiles RELATIVE ${PROJECT_SOURCE_DIR}/tests test-*.cc)
foreach(fullname ${testfiles})
  STRING(REGEX REPLACE "[^/]*/" "" testfile "${fullname}")
  string(REGEX MATCH "^(.*)\\.cc$" dummy ${testfile})
  set(testname ${CMAKE_MATCH_1})
  add_executable(${testname} ${fullname}  $<TARGET_OBJECTS:server> $<TARGET_OBJECTS:common> $<TARGET_OBJECTS:proto>)
  target_compile_definitions(${testname} PRIVATE "BOOST_TEST_DYN_LINK=1")
  target_include_directories(${testname} PRIVATE ${Boost_INCLUDE_DIRS} test-include)
  target_link_libraries(${testname} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${test_library_dependencies})
  add_test(NAME ${testname}
    COMMAND ${testname})
endforeach(fullname)

# individual tests

file(GLOB testfiles RELATIVE ${PROJECT_SOURCE_DIR}/tests test_*.cc)
foreach(fullname ${testfiles})
  STRING(REGEX REPLACE "[^/]*/" "" testfile "${fullname}")
  string(REGEX MATCH "^(.*)\\.cc$" dummy ${testfile})
  set(testname ${CMAKE_MATCH_1})
  add_executable(${testname} ${fullname}  $<TARGET_OBJECTS:server> $<TARGET_OBJECTS:common>)
  target_link_libraries(${testname} ${test_library_dependencies})
endforeach(fullname)

