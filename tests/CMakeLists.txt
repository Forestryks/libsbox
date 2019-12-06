include_directories(include)

set(
    TEST_TARGETS ""
)

macro(libsbox_cpp_test name)
    set(
        TEST_TARGETS "${TEST_TARGETS}" "${name}"
    )
    add_executable(${name} EXCLUDE_FROM_ALL ${name}.cpp)
    target_link_libraries(${name} sbox)
endmacro()

macro(libsbox_ccp_executable name)
    set(
        TEST_TARGETS "${TEST_TARGETS}" "${name}"
    )
    add_executable(${name} EXCLUDE_FROM_ALL ${name}.cpp)
endmacro()

libsbox_cpp_test(test_validate_exit_code)
libsbox_cpp_test(test_validate_term_signal)

add_custom_target(
    tests
    DEPENDS ${TEST_TARGETS} run.py tests.py
)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

add_custom_target(
    test
    DEPENDS tests
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run.py
)
