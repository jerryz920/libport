
cmake_minimum_required (VERSION 2.6)
project (libport)
# The version number.
set (libport_VERSION_MAJOR 1)
set (libport_VERSION_MINOR 0)
add_definitions("-Wall -std=c++11")

find_program(GOTOOL go)
find_package(Boost 1.54 REQUIRED COMPONENTS unit_test_framework system)
find_library(CPPREST cpprest)
find_library(SSL ssl)
find_library(CRYPTO crypto)

add_library(port core.cc log.cc metadata.cc serialization.cc utils.cc)
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories(SYSTEM "/usr/local/include")

# testing
enable_testing()
set(Boost_ADDITIONAL_VERSIONS "1.54" "1.54.0")
set(library_dependencies boost_system crypto ssl cpprest port)

add_executable(test_port_manager test_port_manager.cc)
target_include_directories(test_port_manager PRIVATE ${Boost_INCLUDE_DIRS} test-include)
# indicates the shared library variant
target_compile_definitions(test_port_manager PRIVATE "BOOST_TEST_DYN_LINK=1")
# indicates the link paths, which includes another path pointed library
target_link_libraries(test_port_manager ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(NAME test_port_manager
  COMMAND test_port_manager)

# TODO: is there a way to 
add_executable(test_serialization test_serialization.cc)
target_include_directories(test_serialization PRIVATE ${Boost_INCLUDE_DIRS} test-include)
# indicates the shared library variant
target_compile_definitions(test_serialization PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(test_serialization ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${library_dependencies})
add_test(NAME test_serialization
  COMMAND test_serialization)

add_executable(test_core test_core.cc)
target_include_directories(test_core PRIVATE ${Boost_INCLUDE_DIRS} test-include)
target_compile_definitions(test_core PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(test_core ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${library_dependencies})
add_test(NAME test_core
  COMMAND test_core)

# metadata client test is a separate test with server side option, figure it out.
add_executable(test_metadata test_metadata.cc)
#target_link_libraries(test_metadata boost_system crypto ssl cpprest port)
target_link_libraries(test_metadata ${library_dependencies})

if (NOT GOTOOL)
  MESSAGE("No go binary found, you may want to manual test this")
else (NOT GOTOOL)
  # Run go server first, and then run the test
  add_test(NAME test_metadata
    COMMAND /bin/bash ${PROJECT_SOURCE_DIR}/test_metadata.sh ${PROJECT_SOURCE_DIR})
endif (NOT GOTOOL)



