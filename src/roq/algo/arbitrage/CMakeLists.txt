set(TARGET_NAME ${PROJECT_NAME}-arbitrage)

set(SOURCES factory.cpp simple.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(
  ${TARGET_NAME}
  PRIVATE ${PROJECT_NAME}-utils
          roq-market::roq-market
          roq-cache::roq-cache
          roq-utils::roq-utils
          roq-logging::roq-logging
          roq-api::roq-api
          fmt::fmt)
