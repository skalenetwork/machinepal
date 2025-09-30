cmake_minimum_required(VERSION 3.16)
project(x402proxy LANGUAGES CXX)

# Acknowledge removal of FindBoost; use BoostConfig instead (vcpkg provides it).
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Gather all source files except test sources
file(GLOB APP_SRC_FILES src/*.cpp src/*.h src/datastructures/*.cpp src/datastructures/*.h
        src/exceptions/*.cpp src/exceptions/*.h)
list(FILTER APP_SRC_FILES EXCLUDE REGEX "src/tests/.*")

# Gather test source files
file(GLOB TEST_SRC_FILES src/tests/*.cpp)

# Gather all source files needed for tests (implementation + tests)
set(TEST_ALL_SRC_FILES ${APP_SRC_FILES} ${TEST_SRC_FILES})

# ---- Proxygen / Folly / Wangle ----
find_package(folly    REQUIRED CONFIG)
find_package(Wangle   REQUIRED CONFIG)
find_package(Proxygen REQUIRED CONFIG)
find_package(Boost REQUIRED CONFIG COMPONENTS unit_test_framework system thread)
find_package(CURL  REQUIRED)
find_package(nlohmann_json_schema_validator CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)


# ---- App target ----
add_executable(x402proxy
        main.cpp
        ${APP_SRC_FILES}
)
target_include_directories(x402proxy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set (PROXY_COMMON_LIBS
        CURL::libcurl
        proxygen::proxygenhttpserver
        wangle::wangle
        Folly::folly
        Boost::boost
        Boost::system
        Boost::thread
        Boost::headers
        yaml-cpp::yaml-cpp
        nlohmann_json_schema_validator
)

target_link_libraries(x402proxy
        PRIVATE
        ${PROXY_COMMON_LIBS}
)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    target_compile_options(x402proxy PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(x402proxy PRIVATE -fsanitize=address)
endif()

# ---- Tests: Boost.Test + libcurl ----
include(CTest)
if(BUILD_TESTING)
    enable_testing()

    add_executable(x402_boost_test
            ${TEST_ALL_SRC_FILES}
    )
    target_compile_features(x402_boost_test PRIVATE cxx_std_17)
    target_include_directories(x402_boost_test
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )



    target_link_libraries(x402_boost_test
            PRIVATE
            ${PROXY_COMMON_LIBS}
            Boost::unit_test_framework
    )

    if(ENABLE_ASAN)
        target_compile_options(x402_boost_test PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(x402_boost_test PRIVATE -fsanitize=address)
    endif()

    add_test(NAME x402_boost_test COMMAND x402_boost_test)
endif()
