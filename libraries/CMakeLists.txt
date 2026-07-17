# set relatively new cmake version
cmake_minimum_required(VERSION 3.29)

# Start the library project
project(UnicadoLibs
    VERSION 2.1
    DESCRIPTION "Libraries for UNICADO"
    LANGUAGES CXX
)

# set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# When GCC is used, check if the version is 10.2.0 or higher
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.2.0)
        message(FATAL_ERROR "[${PROJECT_NAME}] -> GCC version 10.2.0 or higher is required!")
    endif()
endif()

# Disable the min and max macros when building with MSVC
if(MSVC)
    add_compile_definitions(NOMINMAX)
endif()

# *optional* Add cppcheck and cpplint
# set(CMAKE_CXX_CPPCHECK "cppcheck")
# set(CMAKE_CXX_CPPLINT "cpplint")

# Option whether to use gnuplot
option(USE_GNUPLOT "Use gnuplot" ON)
# Option for blackbox testing
option(BUILD_BLACKBOXTESTS "Use blackbox testing" OFF)
# Option for unit testing
option(BUILD_UNITTEST "Use unit testing" OFF)
# Option to include system libraries in the package archive
option(PACKAGE_SYSTEM_LIBRARIES "Include system libraries in the package archive" ON)
# Finally decide whether to build static or shared libraries
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Set the output directory for all libraries depending on platform
if (BUILD_SHARED_LIBS)
    if( WIN32 )
        message(STATUS "[${PROJECT_NAME}] -> Windows detected: Building .dll")
    else( UNIX )
        message(STATUS "[${PROJECT_NAME}] -> Unix detected: Building .so")
    endif()
else()
    message(STATUS "[${PROJECT_NAME}] -> Building static libraries")
endif()

# Get Python and pipenv to build the Python packages
set(PYTHON_TARGETS "") # List containing the python targets

#=== Googletest setup ===
if(BUILD_BLACKBOXTESTS OR BUILD_UNITTEST)
    message(STATUS "[${PROJECT_NAME}] -> Building tests for the libraries.")
    # Download and unpack googletest at configure time
    include(FetchContent)
    FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.13.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # *optional* Add gtest to ctest
    # enable_testing()
    # include(GoogleTest)
endif()

# Add library for python bindings
set(PYBIND11_FINDPYTHON ON) # Let pybind11 find the python interpreter
add_subdirectory(extern/pybind11)

# Set the output directory for all libraries -> Use generator expressions to force multi-generator output to the same location
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unicadoRuntimeLibs$<0:>) # .dll for Windows
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unicadoRuntimeLibs$<0:>) # .so for Unix

# Include the library directories based on dependency order
add_subdirectory(aerodynamics)
add_subdirectory(aixml)
add_subdirectory(airfoils)
add_subdirectory(blackboxTest)
add_subdirectory(aircraftGeometry2)
add_subdirectory(atmosphere)
add_subdirectory(coordinateSystemConversion)
add_subdirectory(engine)
add_subdirectory(energyCarriers)
add_subdirectory(liftingLineInterface2)
add_subdirectory(moduleBasics)
add_subdirectory(pymodulepackage)
add_subdirectory(runtimeInfo)
add_subdirectory(standardFiles)
add_subdirectory(svl)
add_subdirectory(unitConversion)

# Add a custom target which installs all python packages if pipenv is available
if(PIPENV)
    message(STATUS "[${PROJECT_NAME}] -> Pipenv is available, use target 'install_python_packages' to install the python packages inside your virtual environment.")
    add_custom_target(install_python_packages 
        COMMAND ${PIPENV} run python -m pip install --force-reinstall ${PYTHON_TARGETS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} # Execute in the top level source directory
        # List the dependencies which have to be build first:
        DEPENDS py11aircraftGeometry2 py11aixml py11coordinateSystemConversion py11unitConversion py11energyCarriers
    )
endif()

# === Package setup ===
# create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/UnicadoLibsConfig.cmake.in
  "${CMAKE_CURRENT_LIST_DIR}/cmake/UnicadoLibsConfig.cmake"
  INSTALL_DESTINATION cmake
)

# create version file
write_basic_package_version_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/UnicadoLibsConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Provide a cmake file which can be used to get a list of all python packages
configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/UnicadoPythonPackages.cmake.in
    ${CMAKE_CURRENT_LIST_DIR}/cmake/UnicadoPythonPackages.cmake
)
