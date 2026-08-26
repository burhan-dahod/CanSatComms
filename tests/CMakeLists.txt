# Add the test executable using your test source file
add_executable(comms_tests
    test_comms.cpp
)

# Link the test executable against your core library and Google Test main runner
target_link_libraries(comms_tests 
    PRIVATE 
        comms_lib 
        GTest::gtest_main
)

# Automatically discover and register individual Google Tests with CTest
include(GoogleTest)
gtest_discover_tests(comms_tests)