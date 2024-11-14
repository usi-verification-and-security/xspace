cmake_minimum_required(VERSION 3.20)
project(XAI-SMT)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# Find OpenSMT2
SET(OPENSMT_HOME CACHE STRING "OpenSMT installation directory")
find_package(OpenSMT 2.6.0 EXACT CONFIG PATHS ${OPENSMT_HOME} NO_DEFAULT_PATH)

if (OpenSMT_FOUND)
else(OpenSMT_FOUND)
    FetchContent_Declare(
        OpenSMT
        GIT_REPOSITORY https://github.com/usi-verification-and-security/opensmt.git
        GIT_TAG v2.8.0
        GIT_SHALLOW true
        GIT_PROGRESS true
    )
    set(BUILD_EXECUTABLES OFF CACHE INTERNAL "")
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")
    set(PACKAGE_TESTS OFF CACHE INTERNAL "")
    message("Getting OpenSMT...")
    FetchContent_MakeAvailable(OpenSMT)
    message("OpenSMT downloaded")
    add_library(OpenSMT::OpenSMT ALIAS OpenSMT-static)
endif(OpenSMT_FOUND)

if (NOT OpenSMT_FOUND)
    # target_compile_definitions(XAI-SMT PUBLIC OPENSMT_LOCAL_BUILD)
    #message("Directories:${opensmt_SOURCE_DIR}; ${opensmt_BINARY_DIR}")
    target_include_directories(OpenSMT-static PUBLIC
    ${opensmt_SOURCE_DIR}/src
    )
    #message("GMP dirs:${GMP_INCLUDE_DIR}")
    # target_include_directories(XAI-SMT PUBLIC ${GMP_INCLUDE_DIR})
endif()


FetchContent_Declare(
        Marabou
        GIT_REPOSITORY https://github.com/NeuralNetworkVerification/Marabou.git
        GIT_TAG 215828c64e624be7917e69e4e873c746d8df25a2
        GIT_SHALLOW true
        GIT_PROGRESS true
    )
FetchContent_MakeAvailable(Marabou)
add_library(Marabou::Marabou ALIAS MarabouHelper)
get_target_property(MARABOU_INCLUDE_DIRS MarabouHelper INCLUDE_DIRECTORIES)
#message(STATUS "Include directories for Marabou: ${MARABOU_INCLUDE_DIRS}")

include_directories(src)

add_executable(XAI-SMT
    src/main.cpp
    src/nn/NNet.cpp
    src/algorithms/BasicVerix.cpp
    # src/algorithms/HypothesisTesting.cpp
    src/experiments/VerixExperiments.cpp
    # src/experiments/VerixHiddenLayerExperiments.cpp
)

target_link_libraries(XAI-SMT PUBLIC OpenSMT::OpenSMT)

target_link_libraries(XAI-SMT PUBLIC Marabou::Marabou)
target_include_directories(XAI-SMT PRIVATE ${MARABOU_INCLUDE_DIRS})
#get_target_property(MY_INCLUDE_DIRS XAI-SMT INCLUDE_DIRECTORIES)
#message(STATUS "Include directories for me: ${MY_INCLUDE_DIRS}")

add_subdirectory(src)
