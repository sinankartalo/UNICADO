# Set name of executable
set(LIBRARY_NAME blackboxTest)


# Include googletest if it is not already included
if(NOT TARGET GTest::gtest)
    message(STATUS "[${LIBRARY_NAME}] -> Including googletest since it is needed by the blackbox test library.")
    # Download and unpack googletest at configure time
    include(FetchContent)
    FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.13.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Add the library
# -> Has to be static, because not all implementations are provided here
add_library(${LIBRARY_NAME} INTERFACE)
add_library( UnicadoLibs::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME} ) # Create an alias target with a namespace alias

# Add the library include directories
target_include_directories(${LIBRARY_NAME}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link the runtime libraries -> Use the actual needed libraries
# GTest is public, so the derived tests see its headers
target_link_libraries(${LIBRARY_NAME}
    INTERFACE
        runtimeInfo
        standardFiles
        aixml
        moduleBasics
        GTest::gtest
)

# Add the export rules
export(TARGETS gtest ${LIBRARY_NAME}
    FILE "${PROJECT_SOURCE_DIR}/cmake/UnicadoLibs${LIBRARY_NAME}Targets.cmake"
    NAMESPACE UnicadoLibs::
)
