cmake_minimum_required(VERSION 3.18)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)

set(DEPENDENCIES_DIR PATH ${CMAKE_SOURCE_DIR}/dependencies)

project(prometheus-mqtt-exporter)

add_executable(prometheus-mqtt-exporter
    ${CMAKE_SOURCE_DIR}/src/Configuration.cpp
    ${CMAKE_SOURCE_DIR}/src/Configuration.h
    ${CMAKE_SOURCE_DIR}/src/HttpServer.cpp
    ${CMAKE_SOURCE_DIR}/src/HttpServer.h
    ${CMAKE_SOURCE_DIR}/src/Main.cpp
    ${CMAKE_SOURCE_DIR}/src/MetricsAccumulator.cpp
    ${CMAKE_SOURCE_DIR}/src/MetricsAccumulator.h
    ${CMAKE_SOURCE_DIR}/src/MetricsPresenter.cpp
    ${CMAKE_SOURCE_DIR}/src/MetricsPresenter.h
    ${CMAKE_SOURCE_DIR}/src/LoggerFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerFactory.h
    ${CMAKE_SOURCE_DIR}/src/MqttClient.cpp
    ${CMAKE_SOURCE_DIR}/src/MqttClient.h
    ${CMAKE_SOURCE_DIR}/src/StateMachine.h
    ${CMAKE_SOURCE_DIR}/src/TaskQueue.cpp
    ${CMAKE_SOURCE_DIR}/src/TaskQueue.h
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