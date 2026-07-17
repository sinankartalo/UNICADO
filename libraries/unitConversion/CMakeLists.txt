# Set the library name
set( LIBRARY_NAME unitConversion )

# Create the library target -> this is a header only library
add_library( ${LIBRARY_NAME} INTERFACE )
add_library( UnicadoLibs::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME} ) # Create an alias target with a namespace alias

# Add the include directories as the public interface
target_sources( ${LIBRARY_NAME}
PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        "include/unitConversion/constants.h"
        "include/unitConversion/unitBase.h"
        "include/unitConversion/unitConversion.h"
        "include/unitConversion/unitConversionConf.h"
)

# Link against dependent libraries
target_link_libraries( ${LIBRARY_NAME} INTERFACE atmosphere)


# Add the install rules
#install(TARGETS ${LIBRARY_NAME} FILE_SET HEADERS)

# Add the export rules
export(TARGETS ${LIBRARY_NAME}
    FILE "${PROJECT_SOURCE_DIR}/cmake/UnicadoLibs${LIBRARY_NAME}Targets.cmake"
    NAMESPACE UnicadoLibs::
)

# === Add python bindings ===
pybind11_add_module( py11${LIBRARY_NAME}
    src/py_unit_conversion.cpp
)

target_include_directories(py11${LIBRARY_NAME} PUBLIC include)

# On Windows the Python Debug builds are not properly supported -> warn about this
if(MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
    message(WARNING "\n[${PROJECT_NAME}] -> Building Python binding of ${LIBRARY_NAME} in Debug Mode on Windows. This is not properly supported!")
endif()

# Set the output directory for the python binding -> Use generator expressions to force multi-generator output to the same location
set_target_properties(py11${LIBRARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyunitconversion/pyunitconversion$<0:>) # .dll for Windows
set_target_properties(py11${LIBRARY_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyunitconversion/pyunitconversion$<0:>) # .so for Unix

# Add the package to the package list for exporting the target
# and propagate the resulting list back to the parent scope
list( APPEND PYTHON_TARGETS ${CMAKE_CURRENT_LIST_DIR}/pyunitconversion)
set( PYTHON_TARGETS ${PYTHON_TARGETS} PARENT_SCOPE )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyunitconversion DESTINATION lib)