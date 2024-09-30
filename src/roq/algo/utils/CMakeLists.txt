set(TARGET_NAME ${PROJECT_NAME}-utils)

set(SOURCES common.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-logging::roq-logging roq-api::roq-api fmt::fmt)
