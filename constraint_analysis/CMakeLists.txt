cmake_minimum_required(VERSION 3.20)

project(constraint_analysis_photo_style LANGUAGES CXX)

find_package(Eigen3 REQUIRED)
find_package(CGAL 5.6 REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB ATMOSPHERE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/atmosphere/src/*.cpp"
)

file(GLOB UNITCONVERSION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/unitConversion/src/*.cpp"
)

file(GLOB ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/engine/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/engine/src/parser/*.cpp"
)

file(GLOB AIXML_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/aixml/src/*.cpp"
)

file(GLOB RUNTIMEINFO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/runtimeInfo/src/*.cpp"
)

file(GLOB STANDARDFILES_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/standardFiles/src/*.cpp"
)

file(GLOB MODULEBASICS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/moduleBasics/src/*.cpp"
)

set(AERODYNAMICS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/aerodynamics/src/aerodynamics/aerodynamics_v3.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/aerodynamics/src/interpolation/interpolation_v2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/aerodynamics/src/solver/solver_v2.cpp"
)

list(FILTER UNITCONVERSION_SOURCES EXCLUDE REGEX ".*py_unit_conversion\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*py.*\.cpp$")
list(FILTER AIXML_SOURCES EXCLUDE REGEX ".*py.*\.cpp$")
list(FILTER AIXML_SOURCES EXCLUDE REGEX ".*main\.cpp$")

add_library(constraint_analysis_lib
    src/constraint_analysis.cpp
    src/ca_functions.cpp
    src/ca_minfinder.cpp
    src/ca_plotting.cpp
    src/io/aerodynamics_xml.cpp
    src/ca_parser.cpp
    src/energy_based/energy_based.cpp
    src/energy_based/energy_based_report.cpp

    ${ATMOSPHERE_SOURCES}
    ${UNITCONVERSION_SOURCES}
    ${ENGINE_SOURCES}
    ${AIXML_SOURCES}
    ${RUNTIMEINFO_SOURCES}
    ${STANDARDFILES_SOURCES}
    ${MODULEBASICS_SOURCES}
    ${AERODYNAMICS_SOURCES}
)

if(WIN32)
    target_compile_definitions(constraint_analysis_lib PRIVATE "_POSIX_THREAD_SAFE_FUNCTIONS")
endif()

target_include_directories(constraint_analysis_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/atmosphere/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/unitConversion/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/engine/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/aixml/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/runtimeInfo/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/standardFiles/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/moduleBasics/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/svl/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/aerodynamics/include
)

target_link_libraries(constraint_analysis_lib
    PUBLIC
        Eigen3::Eigen
        CGAL::CGAL
        gmp
        mpfr
)

add_executable(constraint_analysis_app
    src/main.cpp
)

target_link_libraries(constraint_analysis_app
    PRIVATE
        constraint_analysis_lib
)
