cmake_minimum_required(VERSION 3.18)


# Ensure vcpkg toolchain is applied even when CLion overrides command-line args
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)
endif()

if (DEFINED CMAKE_C_COMPILER_LAUNCHER)
    message(STATUS "Using C compiler launcher: ${CMAKE_C_COMPILER_LAUNCHER}")
endif()

if (DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    message(STATUS "Using CXX compiler launcher: ${CMAKE_CXX_COMPILER_LAUNCHER}")
endif()

# Force re-apply launcher after toolchain initialization
set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_C_COMPILER_LAUNCHER}" CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_CXX_COMPILER_LAUNCHER}" CACHE STRING "" FORCE)


project(machinepay LANGUAGES CXX)

# Enable thread safety globally
add_compile_definitions(
        SQLITE_THREADSAFE=1
        SQLITE_ENABLE_SERIALIZED=1
        SOCI_THREAD_SAFE=ON
)


set(WITH_SQLITE3 ON CACHE BOOL "Build SOCI SQLite3 backend" FORCE)
option(ENABLE_POSTGRESQL "Enable PostgreSQL backend via SOCI" OFF)
if(ENABLE_POSTGRESQL)
    set(WITH_POSTGRESQL ON CACHE BOOL "Build SOCI PostgreSQL backend" FORCE)
endif()


SET(SECP256K1_ENABLE_MODULE_RECOVERY ON)
foreach(submod json-schema-validator yaml-cpp secp256k1 soci)
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
set(APP_SRC_DIRS . config config/subconfigs crypto datastructures db exceptions examples facilitator_clients filesystem
        init  payment payment/datastructures url x402_client x402_protocol x402_server)

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
foreach(pkg folly Wangle Proxygen CURL CLI11 spdlog)
    find_package(${pkg} REQUIRED CONFIG)
endforeach()



find_package(Boost REQUIRED CONFIG COMPONENTS unit_test_framework system thread url locale multiprecision)
find_package(OpenSSL REQUIRED)
find_package(cryptopp CONFIG REQUIRED)

# ---- App target ----
add_executable(machinepay
        main.cpp
        ${APP_SRC_FILES}
)
target_include_directories(machinepay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/external/soci/include)


set (MACHINEPAY_COMMON_LIBS
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
        Boost::multiprecision
        OpenSSL::Crypto
        OpenSSL::SSL
        nlohmann_json_schema_validator
        yaml-cpp
        spdlog::spdlog
        cryptopp::cryptopp
        CLI11::CLI11
        secp256k1
        soci_core
        soci_sqlite3
        soci_postgresql
)

if(ENABLE_POSTGRESQL)
    list(APPEND MACHINEPAY_COMMON_LIBS soci_postgresql)
    target_compile_definitions(machinepay PRIVATE ENABLE_POSTGRESQL)
endif()

target_link_libraries(machinepay
        PRIVATE
        ${MACHINEPAY_COMMON_LIBS}
)

target_compile_options(machinepay PRIVATE -Wno-deprecated-declarations)

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
            ${CMAKE_CURRENT_SOURCE_DIR}/tests)


    target_link_libraries(mptest
            PRIVATE
            ${MACHINEPAY_COMMON_LIBS}
            Boost::unit_test_framework
    )

    if(ENABLE_POSTGRESQL)
        target_compile_definitions(mptest PRIVATE ENABLE_POSTGRESQL)
    endif()

    if(ENABLE_ASAN)
        target_compile_options(mptest PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(mptest PRIVATE -fsanitize=address)
    endif()

    add_test(NAME mptest COMMAND mptest)
endif()

function(enable_warnings target)
    target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wconversion-null
            -Wnull-dereference
    )
endfunction()

enable_warnings(machinepay)
enable_warnings(mptest)

# make everything compile faster
target_precompile_headers(machinepay PRIVATE "src/MachinePayCommon.h")
target_precompile_headers(mptest PRIVATE "src/MachinePayCommon.h")

# ---- CPack/DEB packaging ----
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_CONTACT "Stan Kladko <maintainers@skalenetwork.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Stan Kladko <maintainers@skalenetwork.com>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "MachinePay core and test binaries")
set(CPACK_DEBIAN_PACKAGE_VERSION "1.0")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 11.2)")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
install(TARGETS machinepay mptest DESTINATION usr/bin)
include(CPack)
