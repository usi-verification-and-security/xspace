add_library(xspace OBJECT)

target_sources(xspace
PRIVATE
    common/Bound.cpp
    common/Interval.cpp
    common/Print.cpp
    framework/Framework.cpp
    framework/Preprocess.cpp
    framework/Print.cpp
    framework/Utils.cpp
    framework/expand/Expand.cpp
    framework/expand/strategy/Strategy.cpp
    framework/expand/strategy/AbductiveStrategy.cpp
    framework/expand/strategy/TrialAndErrorStrategy.cpp
    framework/expand/strategy/UnsatCoreStrategy.cpp
    framework/expand/strategy/OpenSMTInterpolationStrategy.cpp
    framework/explanation/Explanation.cpp
    framework/explanation/IntervalExplanation.cpp
    framework/explanation/VarBound.cpp
    framework/explanation/opensmt/FormulaExplanation.cpp
    nn/Dataset.cpp
PUBLIC
)

target_include_directories(xspace SYSTEM
PUBLIC
    ${opensmt_SOURCE_DIR}/src
)

target_compile_options(xspace
PRIVATE
    -Wall -Wextra -pedantic -Wshadow
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
target_compile_options(xspace
PRIVATE
    -Weffc++ -Wno-non-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wmissing-declarations
)
endif()

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
