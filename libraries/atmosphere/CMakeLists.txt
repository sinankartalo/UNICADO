# Set the library name
set( LIBRARY_NAME atmosphere )

# Set the library sources
set( LIBRARY_SOURCES
src/atmosphere.cpp
resources.rc
)

# Create the library target
add_library( ${LIBRARY_NAME} ${LIBRARY_SOURCES} )
add_library( UnicadoLibs::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME} ) # Create an alias target with a namespace alias

# Set the library properties
if( WIN32 )
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "_POSIX_THREAD_SAFE_FUNCTIONS")
endif()

# Options when building a shared library
if ( BUILD_SHARED_LIBS )
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "BUILD_ATMOSPHERE_SHARED" PUBLIC "IMPORT_ATMOSPHERE_SHARED")
endif()

# Add the include directories as the public interface
target_include_directories( ${LIBRARY_NAME} PUBLIC include )

# Add the unitConversion include directory as a private interface to resolve
# the circular dependency between the two libraries
target_include_directories( ${LIBRARY_NAME} PRIVATE ../unitConversion/include )

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
