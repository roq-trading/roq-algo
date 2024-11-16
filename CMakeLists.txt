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

find_package(fmt REQUIRED)
find_package(magic_enum REQUIRED)
find_package(roq-api REQUIRED)
find_package(roq-cache REQUIRED)
find_package(roq-logging REQUIRED)
find_package(roq-market REQUIRED)
find_package(roq-utils REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(unordered_dense REQUIRED)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

if(BUILD_TESTING)
  find_package(Catch2 REQUIRED)
endif()

# include

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)

# sub-projects

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/roq/algo)

if(BUILD_TESTING)
  add_subdirectory(${CMAKE_SOURCE_DIR}/test)
endif()

# benchmark

find_package(benchmark REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/benchmark)

# install

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/roq/
  DESTINATION include/roq
  FILES_MATCHING
  PATTERN "*.hpp")

# config

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-config)

install(FILES ${CMAKE_SOURCE_DIR}/CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME})

set(ROQ_CLIENT_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

export(
  TARGETS ${PROJECT_NAME}
  NAMESPACE ${PROJECT_NAME}::
  FILE ${ROQ_CLIENT_CMAKE_DIR}/${PROJECT_NAME}-config.cmake)

install(
  EXPORT ${PROJECT_NAME}-config
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${ROQ_CLIENT_CMAKE_DIR})
