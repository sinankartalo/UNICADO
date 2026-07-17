# Set the library name
set( LIBRARY_NAME moduleBasics )

# Set the library sources
set( LIBRARY_SOURCES
    src/plot.cpp
    src/report.cpp
    resources.rc
)

# Create the library target
add_library( ${LIBRARY_NAME} ${LIBRARY_SOURCES} )
add_library( UnicadoLibs::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME} ) # Create an alias target with a namespace alias

# Create the library target -> this is for header only library
add_library( strategy INTERFACE )
add_library( UnicadoLibs::strategy ALIAS strategy )

target_sources( strategy 
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            "include/moduleBasics/strategySelector.h"
)


# Set the library properties
if( WIN32 )
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "_POSIX_THREAD_SAFE_FUNCTIONS")
endif()

# Options when building a shared library
if ( BUILD_SHARED_LIBS )
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "BUILD_MODULEBASICS_SHARED" PUBLIC "IMPORT_MODULEBASICS_SHARED")
endif()

# Add the include directories as the public interface
target_include_directories( ${LIBRARY_NAME} PUBLIC include )

# Link against dependent libraries
target_link_libraries( ${LIBRARY_NAME} 
    PUBLIC
        strategy
        runtimeInfo
        aixml
        standardFiles
)

if( BUILD_UNITTEST)
    add_subdirectory(test)
endif()


# Add the install rules
install(TARGETS ${LIBRARY_NAME} EXPORT ${LIBRARY_NAME}Targets
    RUNTIME DESTINATION unicadoRuntimeLibs # Windows
    LIBRARY DESTINATION unicadoRuntimeLibs # Linux
)

# Install rules (header only)
#install(TARGETS strategy FILE_SET HEADERS)

# Add the export rules
export(EXPORT ${LIBRARY_NAME}Targets
    FILE "${PROJECT_SOURCE_DIR}/cmake/UnicadoLibs${LIBRARY_NAME}Targets.cmake"
    NAMESPACE UnicadoLibs::
)

# Export rules (header only)
export(TARGETS strategy
    FILE "${PROJECT_SOURCE_DIR}/cmake/UnicadoLibsstrategyTargets.cmake"
    NAMESPACE UnicadoLibs::
)
