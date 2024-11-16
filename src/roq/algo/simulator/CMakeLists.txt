set(TARGET_NAME ${PROJECT_NAME}-simulator)

set(SOURCES config.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-utils::roq-utils roq-logging::roq-logging fmt::fmt)
