include_directories(include)

set(
    TEST_TARGETS ""
)

macro(libsbox_cpp_test name)
    set(
        TEST_TARGETS "${TEST_TARGETS}" "${name}"
    )
    add_executable(${name} EXCLUDE_FROM_ALL ${name}.cpp)
    target_link_libraries(${name} libsbox::sbox-static)
endmacro()

macro(libsbox_ccp_executable name)
    set(
        TEST_TARGETS "${TEST_TARGETS}" "${name}"
    )
    add_executable(${name} EXCLUDE_FROM_ALL ${name}.cpp)
endmacro()

libsbox_cpp_test(test_exit_code)
libsbox_cpp_test(test_term_signal)
libsbox_cpp_test(test_time_limit)
libsbox_cpp_test(test_wall_time_limit)
libsbox_cpp_test(test_time_usage)
libsbox_cpp_test(test_wall_time_usage)
libsbox_cpp_test(test_memory_limit)
libsbox_cpp_test(test_memory_usage)

add_custom_target(
    build_tests
    DEPENDS ${TEST_TARGETS} run.py tests.py
)

add_custom_target(
    tests
    DEPENDS build_tests
    COMMAND unshare -fp python3 ${CMAKE_CURRENT_SOURCE_DIR}/run.py
)

add_custom_target(
    bundled_tests
    DEPENDS build_tests
    COMMAND unshare -fp python3 ${CMAKE_CURRENT_SOURCE_DIR}/run.py --use-bundled
)