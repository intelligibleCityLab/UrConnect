# UrConnect 项目构建配置
cmake_minimum_required(VERSION 3.13.0)
project(UrConnect)

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
    execute_process(COMMAND make_version_header.bat WORKING_DIRECTORY depthmapX)
endif()

# 策略设置
cmake_policy(SET CMP0076 NEW)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

# Dependency roots can be supplied with -DBOOST_ROOT=... and -DQT5_ROOT=...
set(BOOST_ROOT "" CACHE PATH "Root directory of the Boost installation")
set(QT5_ROOT "" CACHE PATH "Root directory of the Qt 5 installation")

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

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")
endif()

# 子目录构建
add_subdirectory(genlib)
add_subdirectory(mgraph440)
add_subdirectory(SNDAApp/lib)
add_subdirectory(salalib)
add_subdirectory(depthmapX)

