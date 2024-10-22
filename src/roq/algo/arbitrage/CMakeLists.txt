set(TARGET_NAME ${PROJECT_NAME}-arbitrage)

set(SOURCES factory.cpp instrument.cpp simple.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-cache::roq-cache roq-utils::roq-utils roq-logging::roq-logging roq-api::roq-api fmt::fmt)
