# Convert the module sources to a path found by CMake
list(TRANSFORM LIBRARY_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/../ OUTPUT_VARIABLE TEST_SOURCES)

# ==============================================
# Add the test executable
#
# *** IMPORTANT ***
# -> Change *.cpp files according to the tests
# ==============================================
add_executable(test_${LIBRARY_NAME}
    test_standardFiles.cpp
    ${TEST_SOURCES}
)

# Link the test executable against UNICADO libs and gtest
target_link_libraries( test_${LIBRARY_NAME}
    PRIVATE
        standardFiles
        aixml
        runtimeInfo
        GTest::gtest_main
)

# Set compile definitions for the test executable
target_compile_options( test_${LIBRARY_NAME} PRIVATE "-w") # Skip warnings

# The the original source folder as includes, to that the test can access the
# original source files
target_include_directories( test_${LIBRARY_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/
		${CMAKE_CURRENT_SOURCE_DIR}/../include/
)

# Build the test runner into the current directory
set_target_properties( test_${LIBRARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

# Add a custom target to run the tests with cmake
add_custom_target(runtest_${LIBRARY_NAME}
        # COMMAND CTEST_OUTPUT_ON_FAILURE=1 GTEST_COLOR=1
        COMMAND test_${LIBRARY_NAME}
        DEPENDS test_${LIBRARY_NAME})

# *optional* Add the test executable to ctest
# gtest_discover_tests(test_${LIBRARY_NAME})

# Set compile options according to operating system
if( WIN32 )
    target_compile_definitions( test_${LIBRARY_NAME} PRIVATE "_POSIX_THREAD_SAFE_FUNCTIONS;BUILD_STANDARDFILES_DLL")
elseif( UNIX )
    target_compile_definitions( test_${LIBRARY_NAME} PRIVATE "BUILD_STANDARDFILES_SO")
endif()
