cmake_minimum_required(VERSION 3.12)

# cmake

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# config

include(RoqConfig)

# version (using git tag)

include(GetGitRepoVersion)

message("Using GIT_REPO_VERSION=${GIT_REPO_VERSION}")

# project

project(roq-algo VERSION ${GIT_REPO_VERSION})

# filesystem

include(GNUInstallDirs)

# dependencies

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(roq-api REQUIRED)
find_package(roq-client REQUIRED)
find_package(roq-flags REQUIRED)
find_package(roq-logging REQUIRED)
find_package(roq-market REQUIRED)

if(UNIX AND NOT APPLE)
  set(RT_LIBRARIES rt)
else()
  set(RT_LIBRARIES)
endif()

# autogen

find_program(ROQ_AUTOGEN roq-autogen REQUIRED)

set(TEMPLATE_DIR ${CMAKE_SOURCE_DIR}/scripts/templates)

# clang-format

find_program(CLANG_FORMAT clang-format REQUIRED)

# include

include_directories(
  ${CMAKE_SOURCE_DIR}/src
  # >>> autogen >>>
  ${CMAKE_BINARY_DIR}/src)

# sub-projects

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/roq/algo)

# test

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

find_package(Catch2 REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/test)

# benchmark

find_package(benchmark REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/benchmark)
