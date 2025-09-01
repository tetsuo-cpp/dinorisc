# End-to-end tests for DinoRISC
cmake_minimum_required(VERSION 3.20)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(NAME E2ETests
    COMMAND Python3::Interpreter -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/test_e2e.py -v -s
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Add a custom target for running just the e2e tests
add_custom_target(test-e2e
    COMMAND Python3::Interpreter -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/test_e2e.py -v -s
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running end-to-end tests"
    DEPENDS dinorisc
)

message(STATUS "End-to-end tests enabled")
