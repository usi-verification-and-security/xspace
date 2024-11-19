add_library(xspace OBJECT)

target_sources(xspace
PRIVATE
    common/Bound.cpp
    explanation/Explanation.cpp
    framework/Framework.cpp
    framework/Print.cpp
    framework/expand/Expand.cpp
    framework/expand/strategy/Strategy.cpp
    nn/Dataset.cpp
PUBLIC
)

target_include_directories(xspace
PUBLIC
    ${opensmt_SOURCE_DIR}/src
)

add_executable(XSpace-bin
    bin/main.cpp
    ${SOURCE_DIR}/nn/NNet.cpp
    ${SOURCE_DIR}/verifiers/opensmt/OpenSMTVerifier.cpp
)

set_target_properties(XSpace-bin
PROPERTIES
    OUTPUT_NAME xspace
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_link_libraries(XSpace-bin
PUBLIC
    xspace
    OpenSMT::OpenSMT
)
