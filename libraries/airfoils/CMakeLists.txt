# Set the library name
set( LIBRARY_NAME airfoils )

# Set the library sources
set( LIBRARY_SOURCES
    src/airfoils.cpp
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
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "BUILD_AIRFOILS_SHARED" PUBLIC "IMPORT_AIRFOILS_SHARED")
endif()

# Add the include directories as the public interface
target_include_directories( ${LIBRARY_NAME} PUBLIC include )

# Add the unitConversion include directory as a private interface to resolve
# the circular dependency between the two libraries
target_link_libraries( ${LIBRARY_NAME}
    PUBLIC    
        standardFiles
    PRIVATE 
        unitConversion
        aircraftGeometry2
)

if( BUILD_UNITTEST)
    add_subdirectory(test)
endif()

# === Add a demo executable ===
add_executable( demo_${LIBRARY_NAME} ${LIBRARY_SOURCES} src/demo.cpp)
target_link_libraries( demo_${LIBRARY_NAME}
    PRIVATE
        unitConversion
        aircraftGeometry2
        standardFiles
)

target_include_directories( demo_${LIBRARY_NAME}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

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
