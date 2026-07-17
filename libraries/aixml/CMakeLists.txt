# Set the library name
set( LIBRARY_NAME aixml )

# Set the library sources
set( LIBRARY_SOURCES
    src/node.cpp
    src/tinystr.cpp
    src/tinyxml.cpp
    src/tinyxmlerror.cpp
    src/tinyxmlparser.cpp
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
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "BUILD_AIXML_SHARED" PUBLIC "IMPORT_AIXML_SHARED")
endif()

# Add the include directories as the public interface
target_include_directories( ${LIBRARY_NAME} PUBLIC include )

# Link against dependent libraries
target_link_libraries( ${LIBRARY_NAME} 
    PUBLIC
        runtimeInfo
        unitConversion
        standardFiles
)

# Add the tests if enabled
if(BUILD_UNITTEST)
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

# Add python bindings
pybind11_add_module( py11${LIBRARY_NAME}
    src/node.cpp
    src/tinystr.cpp
    src/tinyxml.cpp
    src/tinyxmlerror.cpp
    src/tinyxmlparser.cpp
    src/py_aixml.cpp
)
target_include_directories( py11${LIBRARY_NAME} PUBLIC include)
target_link_libraries( py11${LIBRARY_NAME} PRIVATE UnicadoLibs::unitConversion)

# On Windows the Python Debug builds are not properly supported -> warn about this
if(MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
    message(WARNING "\n[${PROJECT_NAME}] -> Building Python binding of ${LIBRARY_NAME} in Debug Mode on Windows. This is not properly supported!")
endif()

# Set the output directory for the python binding -> Use generator expressions to force multi-generator output to the same location
set_target_properties(py11${LIBRARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyaixml/pyaixml$<0:>) # .dll for Windows
set_target_properties(py11${LIBRARY_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyaixml/pyaixml$<0:>) # .so for Unix

# Add the package to the package list for exporting the target
# and propagate the resulting list back to the parent scope
list( APPEND PYTHON_TARGETS ${CMAKE_CURRENT_LIST_DIR}/pyaixml )
set( PYTHON_TARGETS ${PYTHON_TARGETS} PARENT_SCOPE )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyaixml DESTINATION lib)