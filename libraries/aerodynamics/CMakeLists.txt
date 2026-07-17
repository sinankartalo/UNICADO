# Set the library name
set( LIBRARY_NAME aerodynamics )

# Set the library sources
set( LIBRARY_SOURCES
    src/aerodynamics/aerodynamics_v3.cpp
    src/interpolation/interpolation_v2.cpp
    src/solver/solver_v2.cpp
    resources.rc
)

# Create the library target
add_library( ${LIBRARY_NAME} ${LIBRARY_SOURCES}  )
add_library( UnicadoLibs::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME} ) # Create an alias target with a namespace alias

# Set the library properties
if( WIN32 )
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "_POSIX_THREAD_SAFE_FUNCTIONS")
endif()

# Options when building a shared library
if ( BUILD_SHARED_LIBS )
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "BUILD_AERODYNAMICS_SHARED" PUBLIC "IMPORT_AERODYNAMICS_SHARED")
endif()

# Add the include directories as the public interface
target_include_directories( ${LIBRARY_NAME} PUBLIC include )

find_package(CGAL 5.6 REQUIRED )
find_package(Eigen3 REQUIRED)

target_include_directories(${LIBRARY_NAME}
    PUBLIC
        ${EIGEN3_INCLUDE_DIR}
)
# Link against dependent libraries
target_link_libraries( ${LIBRARY_NAME} 
    PUBLIC
        CGAL::CGAL
        aixml
        runtimeInfo
        atmosphere
        aircraftGeometry2
)

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

# === Add python bindings ===
pybind11_add_module( py${LIBRARY_NAME}
    src/aerodynamics/aerodynamics_v3.cpp
    src/interpolation/interpolation_v2.cpp
    src/solver/solver_v2.cpp
    src/py_aerodynamics3.cpp )
target_include_directories( py${LIBRARY_NAME} PUBLIC include)
set_target_properties(aerodynamics PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(py${LIBRARY_NAME} PUBLIC
    CGAL::CGAL
    aixml
    runtimeInfo
    atmosphere
    aircraftGeometry2
)
target_include_directories(py${LIBRARY_NAME}
    PUBLIC
        ${EIGEN3_INCLUDE_DIR}
)

# On Windows the Python Debug builds are not properly supported -> warn about this
if(MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
    message(WARNING "\n[${PROJECT_NAME}] -> Building Python binding of ${LIBRARY_NAME} in Debug Mode on Windows. This is not properly supported!")
endif()

# Set the output directory for the python binding -> Use generator expressions to force multi-generator output to the same location
set_target_properties(py${LIBRARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyaerodynamics/pyaerodynamics) # .dll for Windows
set_target_properties(py${LIBRARY_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyaerodynamics/pyaerodynamics) # .so for Unix

# Add the package to the package list for exporting the target
# and propagate the resulting list back to the parent scope
list( APPEND PYTHON_TARGETS ${CMAKE_CURRENT_LIST_DIR}/pyaerodynamics )
set( PYTHON_TARGETS ${PYTHON_TARGETS} PARENT_SCOPE )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyaerodynamics DESTINATION lib)