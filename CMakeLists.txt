# This file is added just for convenience for those projects
# that already use CMake in their activities.
#
# This library is header only, so no need to build it
# and threre in NO REQUIREMENT to use CMake: 
# one can just copy headers to their project
# and use any build system one likes.


cmake_minimum_required(VERSION 3.27)
project(InstantRTOS VERSION 0.1.0 LANGUAGES CXX)

# Turn ALLOW_INSTANTRTOS_DEVELOPMENT ON once modifying InstantRTOS
# (remember this is cached variable, 
#  so it will stay the same until explicitly changed.
#  For VSCode you can go F1 of Ctrl+Shift+P, then "CMake: Edit CMake Cache" 
#  or "CMake: Edit CMake Cache (UI)" and change the value there,
#  or just delete CMakeCache.txt file in the build directory to start from scratch)
option(
    ALLOW_INSTANTRTOS_DEVELOPMENT
    "Used when developing/testing InstantRTOS" 
    ON
)


# The only purpose here is to populate include directories
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 11  # This is the minimum requirement
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


if(ALLOW_INSTANTRTOS_DEVELOPMENT)
    # While in VSCode, use recommended extentions from extensions.json,
    # they add context menus and buttons into status bar.
    # To see the list of targets and select the one to run
    # of provide --config ConfigurationName in VSCode's
    # click "Activity Bar" on the left, then go "CMake" icon.
    # Find Build/Debug/Run buttons in the status bar below
    # See also https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/debug-launch.md

    if(MSVC)
        # Remember _HAS_STATIC_RTTI=0 shall also do /GR-

        # uncomment below to have PDB in addition to execitable even in release 
        # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "ProgramDatabase")
        # add_link_options("/DEBUG:FULL")
    endif()

    # Optionally it is possible to run tests
    include(CTest)
    if( BUILD_TESTING )
        # Go with doctest as the most natural choice for a C++ project ))

        # The only "external" dependency is when testing with doctest
        include(FetchContent)
        FetchContent_Declare(
            doctest
            GIT_REPOSITORY https://github.com/doctest/doctest.git
            GIT_TAG        master # or use a specific tag instead of master
        )
        FetchContent_MakeAvailable(doctest)

        # Remember you can eiter run tests with CTest
        # of just run (debug) the executable directly from the IDE
        add_subdirectory(tests)
    endif()
endif()
