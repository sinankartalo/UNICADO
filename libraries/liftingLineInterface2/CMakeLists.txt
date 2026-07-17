# Set the library name
set( LIBRARY_NAME liftingLineInterface2 )

# Set the library sources
set( LIBRARY_SOURCES
    src/liftingLineInput.cpp
    src/liftingLineInterface.cpp
    src/liftingLineOutput.cpp
    src/liftingLinePolar.cpp
    src/liftingLineSettings.cpp
    src/liftingLineWing.cpp
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
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "BUILD_LIFTINGLINEINTERFACE_SHARED" PUBLIC "IMPORT_LIFTINGLINEINTERFACE_SHARED")
endif()

# Add the include directories as the public interface
target_include_directories( ${LIBRARY_NAME} PUBLIC include )

# Link against dependent libraries
target_link_libraries( ${LIBRARY_NAME} 
    PRIVATE
        aixml
        runtimeInfo
        standardFiles
        unitConversion
		aircraftGeometry2
		moduleBasics
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
