add_subdirectory(arbitrage)
add_subdirectory(matcher)
add_subdirectory(reporter)
add_subdirectory(simulator)
add_subdirectory(strategy)
add_subdirectory(tools)

set(TARGET_NAME ${PROJECT_NAME})

set(SOURCES main.cpp)

add_library(${TARGET_NAME} SHARED ${SOURCES})

target_link_libraries(
  ${TARGET_NAME}
  INTERFACE roq-api::roq-api magic_enum::magic_enum
  PUBLIC roq-cache::roq-cache roq-utils::roq-utils fmt::fmt
  PRIVATE ${PROJECT_NAME}-arbitrage ${PROJECT_NAME}-matcher ${PROJECT_NAME}-reporter ${PROJECT_NAME}-simulator ${PROJECT_NAME}-strategy ${PROJECT_NAME}-tools)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()
