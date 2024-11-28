set(TARGET_NAME ${PROJECT_NAME}-matcher)

set(SOURCES factory.cpp queue_position_simple.cpp simple.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME}-tools roq-logging::roq-logging roq-api::roq-api fmt::fmt)
