# Set the library name
set( LIBRARY_NAME svl )

# Set the library sources
set( LIBRARY_SOURCES
    src/LibSVL.cpp 
    src/Basics.cpp 
    resources.rc
)

# Create the library target
add_library( ${LIBRARY_NAME} STATIC ${LIBRARY_SOURCES} )
add_library( UnicadoLibs::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME} ) # Create an alias target with a namespace alias

# Add the include directories as the public interface
target_include_directories( ${LIBRARY_NAME} PUBLIC include )

# Add the __CMATH__ symbol to not provide the abs function since gcc comes with it
target_compile_definitions( ${LIBRARY_NAME} PUBLIC __CMATH__)

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
