cmake_minimum_required(VERSION 3.18)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)

set(DEPENDENCIES_DIR PATH ${CMAKE_SOURCE_DIR}/dependencies)

project(prometheus-mqtt-exporter)

add_executable(prometheus-mqtt-exporter
    src/main.cpp
)

target_include_directories(prometheus-mqtt-exporter
    PRIVATE
        ${DEPENDENCIES_DIR}/fmt/include
        ${DEPENDENCIES_DIR}/json/include
        ${DEPENDENCIES_DIR}/spdlog/include
)

target_link_libraries(prometheus-mqtt-exporter
    PRIVATE
        microhttpd
        mosquitto
)