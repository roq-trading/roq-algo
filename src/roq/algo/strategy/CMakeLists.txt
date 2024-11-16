set(TARGET_NAME ${PROJECT_NAME}-strategy)

set(SOURCES config.cpp factory.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME}-arbitrage roq-logging::roq-logging fmt::fmt)
