set(TARGET_NAME ${PROJECT_NAME}-tools)

set(SOURCES market_data.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-market::roq-market roq-cache::roq-cache roq-logging::roq-logging roq-api::roq-api fmt::fmt)
