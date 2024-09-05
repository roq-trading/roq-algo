set(TARGET_NAME ${PROJECT_NAME}-benchmark)

set(SOURCES simple.cpp main.cpp)

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME}-tools roq-client::roq-client roq-logging::roq-logging benchmark::benchmark)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

install(TARGETS ${TARGET_NAME})

add_custom_target(
  benchmark
  COMMAND ${TARGET_NAME}
  DEPENDS ${TARGET_NAME}
  VERBATIM)
