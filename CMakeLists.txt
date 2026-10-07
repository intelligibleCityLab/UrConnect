# UrConnect 项目构建配置
cmake_minimum_required(VERSION 3.14.0)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
project(UrConnect VERSION 0.2.0)

include(CTest)
option(URCONNECT_BUILD_GUI "Build the Qt desktop application" ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 编译器警告设置
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall -Wextra")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /EHsc")
    # Treat source files as UTF-8 encoded (avoids misinterpreting multi-byte chars)
    add_compile_options(/utf-8)
    execute_process(COMMAND make_version_header.bat WORKING_DIRECTORY depthmapX)
endif()

# 策略设置
cmake_policy(SET CMP0076 NEW)
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

# 包含目录
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

# Dependency roots can be supplied with -DBOOST_ROOT=... and -DQT5_ROOT=...
set(BOOST_ROOT "" CACHE PATH "Root directory of the Boost installation")
set(QT5_ROOT "" CACHE PATH "Root directory of the Qt 5 installation")

if(QT5_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${QT5_ROOT}")
endif()

if(BOOST_ROOT)
    if(EXISTS "${BOOST_ROOT}/include/boost")
        set(URCONNECT_BOOST_INCLUDE_DIR "${BOOST_ROOT}/include")
    else()
        set(URCONNECT_BOOST_INCLUDE_DIR "${BOOST_ROOT}")
    endif()
endif()

# 版本信息生成
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    # Get the current working branch
    execute_process(
      COMMAND git rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE APP_BRANCH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the latest abbreviated commit hash of the working branch
    execute_process(
      COMMAND git log -1 --format=%h
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE APP_COMMIT
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(APP_BRANCH "unknown")
    set(APP_COMMIT "unknown")
endif()

string(TIMESTAMP APP_DATE %Y-%m-%d)

# 生成版本头文件
configure_file("${CMAKE_SOURCE_DIR}/version_defs.h.in" "${CMAKE_BINARY_DIR}/version_defs.h" @ONLY)

# Shapelib and the CLI can be built without Qt for headless CI and servers.
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")
endif()

# ============ 第三方依赖：spdlog 日志库 ============
# 通过 FetchContent 自动下载 spdlog（header-only 模式更易集成）
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
    GIT_SHALLOW    TRUE
)
# 让 spdlog 编译为静态库，避免符号冲突
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build shared spdlog" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "Build spdlog examples" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "Build spdlog tests" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "Build spdlog benchmarks" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "Generate spdlog install target" FORCE)
FetchContent_MakeAvailable(spdlog)

# 子目录构建
add_subdirectory(genlib)
add_subdirectory(mgraph440)
add_subdirectory(SNDAApp/lib)
add_subdirectory(cli)

if(URCONNECT_BUILD_GUI)
    add_subdirectory(genlib)
    add_subdirectory(mgraph440)
    add_subdirectory(salalib)
    add_subdirectory(depthmapX)
endif()
