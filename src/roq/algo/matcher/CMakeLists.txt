set(TARGET_NAME ${PROJECT_NAME}-matcher)

set(SOURCES factory.cpp order.cpp simple.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-market::roq-market roq-cache::roq-cache roq-api::roq-api fmt::fmt)
