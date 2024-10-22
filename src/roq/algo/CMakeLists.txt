add_subdirectory(arbitrage)
add_subdirectory(matcher)
add_subdirectory(reporter)
add_subdirectory(strategy)
add_subdirectory(tools)

set(TARGET_NAME ${PROJECT_NAME})

set(SOURCES main.cpp)

add_library(${TARGET_NAME} SHARED ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME}-matcher ${PROJECT_NAME}-tools ${PROJECT_NAME}-arbitrage ${PROJECT_NAME}-strategy
                                             ${PROJECT_NAME}-reporter fmt::fmt)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()
