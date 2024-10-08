set(TARGET_NAME ${PROJECT_NAME}-collector)

set(SOURCES factory.cpp summary.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-logging::roq-logging roq-api::roq-api fmt::fmt)
