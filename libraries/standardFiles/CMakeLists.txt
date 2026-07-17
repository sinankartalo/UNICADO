# Set the library name
set( LIBRARY_NAME standardFiles )

# Set the library sources
set( LIBRARY_SOURCES
    src/functions.cpp
    resources.rc
)

# Add the shared library
add_library( ${LIBRARY_NAME} ${LIBRARY_SOURCES} )
add_library( UnicadoLibs::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME} ) # Create an alias target with a namespace alias

# Set the library properties
if( WIN32 )
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "_POSIX_THREAD_SAFE_FUNCTIONS")
endif()

# Options when building a shared library
if ( BUILD_SHARED_LIBS )
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "BUILD_STANDARDFILES_SHARED" PUBLIC "IMPORT_STANDARDFILES_SHARED")
endif()

# Add the include directories as the public interface
target_include_directories( ${LIBRARY_NAME} PUBLIC include )

find_package(Eigen3 REQUIRED)

# Link against the needed libraries
target_link_libraries( ${LIBRARY_NAME}
    PRIVATE
        aixml
        runtimeInfo
    PUBLIC
        Eigen3::Eigen
        svl
        unitConversion
)

# Unittests
if( BUILD_UNITTEST )
    add_subdirectory(test)
endif()

# Add the install rules
install(TARGETS ${LIBRARY_NAME} EXPORT ${LIBRARY_NAME}Targets
    RUNTIME DESTINATION unicadoRuntimeLibs # Windows
    LIBRARY DESTINATION unicadoRuntimeLibs # Linux
)

# Add the export rules
export(EXPORT ${LIBRARY_NAME}Targets
    FILE "${PROJECT_SOURCE_DIR}/cmake/UnicadoLibs${LIBRARY_NAME}Targets.cmake"
    NAMESPACE UnicadoLibs::
)
