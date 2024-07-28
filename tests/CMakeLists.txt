# Tests to ensure the correctness of the library 

add_executable(InstantRTOS_tests
    test_main.cpp
    test_InstantTimer.cpp
    test_InstantCallback.cpp
    test_InstantCoroutine.cpp
    test_InstantDebounce.cpp
    test_InstantDelegate.cpp
    test_InstantIntrusiveList.cpp
    test_InstantMemory.cpp
)
set_target_properties(InstantRTOS_tests PROPERTIES
    CXX_STANDARD 11  # This is the minimum requirement
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_link_libraries(InstantRTOS_tests
    PRIVATE
        InstantRTOS # the library to be tested
        doctest::doctest # use doctest as the testing framework
) 

# Ensure CTest will discover and run the tests
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(InstantRTOS_tests)
