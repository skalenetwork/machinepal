cmake_minimum_required(VERSION 3.16)
project(machinepay LANGUAGES CXX)

foreach(submod json-schema-validator yaml-cpp)
    add_subdirectory(external/${submod})
endforeach()

# Acknowledge removal of FindBoost; use BoostConfig instead (vcpkg provides it).
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Gather all source files except test sources
set(APP_SRC_DIRS . datastructures exceptions config facilitator_clients init x402_server x402_client
        filesystem crypto x402_protocol)

set(APP_SRC_FILES)
foreach(dir ${APP_SRC_DIRS})
    file(GLOB APP_SRC_TMP src/${dir}/*.cpp ${dir}/*.h)
    list(APPEND APP_SRC_FILES ${APP_SRC_TMP})
endforeach()

list(FILTER APP_SRC_FILES EXCLUDE REGEX "src/tests/.*")

# Gather test source files
file(GLOB TEST_SRC_FILES src/tests/*.cpp)

# Gather all source files needed for tests (implementation + tests)
set(TEST_ALL_SRC_FILES ${APP_SRC_FILES} ${TEST_SRC_FILES})


# ---- Dependencies ----
foreach(pkg folly Wangle Proxygen CURL nlohmann_json_schema_validator yaml-cpp CLI11 spdlog)
    find_package(${pkg} REQUIRED CONFIG)
endforeach()


find_package(Boost REQUIRED CONFIG COMPONENTS unit_test_framework system thread url locale)


# ---- App target ----
add_executable(machinepay
        main.cpp
        ${APP_SRC_FILES}
)
target_include_directories(machinepay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(machinepay PRIVATE CLI11::CLI11)

set (PROXY_COMMON_LIBS
        CURL::libcurl
        proxygen::proxygenhttpserver
        wangle::wangle
        Folly::folly
        Boost::boost
        Boost::system
        Boost::thread
        Boost::headers
        Boost::locale
        Boost::url
        nlohmann_json_schema_validator
        yaml-cpp
        spdlog::spdlog
)

target_link_libraries(machinepay
        PRIVATE
        ${PROXY_COMMON_LIBS}
)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    target_compile_options(machinepay PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(machinepay PRIVATE -fsanitize=address)
endif()

# ---- Tests: Boost.Test + libcurl ----
include(CTest)
if(BUILD_TESTING)
    enable_testing()

    add_executable(mptest
            ${TEST_ALL_SRC_FILES}
    )
    target_compile_features(mptest PRIVATE cxx_std_17)
    target_include_directories(mptest
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )


    target_link_libraries(mptest
            PRIVATE
            ${PROXY_COMMON_LIBS}
            Boost::unit_test_framework
    )

    if(ENABLE_ASAN)
        target_compile_options(machinepayt PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(machinepayt PRIVATE -fsanitize=address)
    endif()

    add_test(NAME machinepayt COMMAND machinepayt)
endif()
