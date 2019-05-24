cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(libsbox)

macro(make_libsbox_cpp_test name)
    add_executable(test_${name} ${name}.cpp)
    target_link_libraries(test_${name} sbox)
endmacro()

macro(run_libsbox_cpp_test name cmd)
    message(${cmd})
    add_test(
        NAME ${name}
        COMMAND test_${cmd} ${ARGN}
    )
endmacro()

make_libsbox_cpp_test(exited)
run_libsbox_cpp_test(
    zero_exit
    exited invoke 0
)
run_libsbox_cpp_test(
    nonzero_exit
    exited invoke 255
)

make_libsbox_cpp_test(signaled)
run_libsbox_cpp_test(
    raise_sigkill
    signaled invoke 9 # SIGKILL
)

make_libsbox_cpp_test(time_limit_exceeded)
run_libsbox_cpp_test(
    endless_while_time_limit_exceeded
    time_limit_exceeded invoke
)

make_libsbox_cpp_test(wall_time_limit_exceeded)
run_libsbox_cpp_test(
    long_sleep_wall_time_limit_exceeded
    wall_time_limit_exceeded invoke
)

make_libsbox_cpp_test(stack_overflow_oom_killed)
run_libsbox_cpp_test(
    stack_overflow_oom_killed
    stack_overflow_oom_killed invoke
)

make_libsbox_cpp_test(heap_overflow_oom_killed)
run_libsbox_cpp_test(
    heap_overflow_oom_killed
    heap_overflow_oom_killed invoke
)

make_libsbox_cpp_test(large_data_exec_target)
make_libsbox_cpp_test(large_data_oom_killed)
run_libsbox_cpp_test(
    large_data_oom_killed
    large_data_oom_killed invoke $<TARGET_FILE:test_large_data_exec_target>
)
