# Set the library name
set( LIBRARY_NAME aircraftGeometry2 )

# Set the library sources
set( LIBRARY_SOURCES
    src/builder/acxml.cpp
    src/builder/builder.cpp
    src/geometry/entity3d.cpp
    src/geometry/section.cpp
    src/geometry/surface.cpp
    src/geometry/factory.cpp
    src/processing/transform.cpp
    src/processing/measure.cpp
    src/io/dat.cpp
    src/io/convert.cpp
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
    target_compile_definitions( ${LIBRARY_NAME} PRIVATE "BUILD_AIRCRAFTGEOMETRY_SHARED" PUBLIC "IMPORT_AIRCRAFTGEOMETRY_SHARED")
endif()

# Add the include directories as the public interface
target_include_directories( ${LIBRARY_NAME} PUBLIC include )

# Find required packages
find_package(Eigen3 REQUIRED)
find_package(CGAL 5.6 REQUIRED )

# Link against dependent libraries
target_link_libraries( ${LIBRARY_NAME} 
    PUBLIC
        Eigen3::Eigen
        CGAL::CGAL
        aixml
)

# === Add a demo executable ===
add_executable( demo_${LIBRARY_NAME} ${LIBRARY_SOURCES} src/demo.cpp )
target_link_libraries( demo_${LIBRARY_NAME} 
    PRIVATE
        aixml
        Eigen3::Eigen
        CGAL::CGAL
)
target_include_directories( demo_${LIBRARY_NAME} 
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# Add testing if enabled
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
pybind11_add_module( py11${LIBRARY_NAME}
    src/builder/acxml.cpp
    src/builder/builder.cpp
    src/geometry/entity3d.cpp
    src/geometry/section.cpp
    src/geometry/factory.cpp
    src/io/dat.cpp
    src/io/convert.cpp
    src/processing/transform.cpp
    src/processing/measure.cpp
    src/py_aircraft_geometry.cpp )
target_include_directories( py11${LIBRARY_NAME} PUBLIC include)
set_target_properties(aixml PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(py11${LIBRARY_NAME} PRIVATE Eigen3::Eigen CGAL::CGAL aixml)

# On Windows the Python Debug builds are not properly supported -> warn about this
if(MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
    message(WARNING "\n[${PROJECT_NAME}] -> Building Python binding of ${LIBRARY_NAME} in Debug Mode on Windows. This is not properly supported!")
endif()

# Set the output directory for the python binding -> Use generator expressions to force multi-generator output to the same location
set_target_properties(py11${LIBRARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyaircraftgeometry2/pyaircraftgeometry2$<0:>) # .dll for Windows
set_target_properties(py11${LIBRARY_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyaircraftgeometry2/pyaircraftgeometry2$<0:>) # .so for Unix

# Add the package to the package list for exporting the target
# and propagate the resulting list back to the parent scope
list( APPEND PYTHON_TARGETS ${CMAKE_CURRENT_LIST_DIR}/pyaircraftgeometry2 )
set( PYTHON_TARGETS ${PYTHON_TARGETS} PARENT_SCOPE )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pyaircraftgeometry2 DESTINATION lib)