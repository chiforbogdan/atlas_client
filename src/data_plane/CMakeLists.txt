cmake_minimum_required(VERSION 2.8)

project(data_plane)

find_package(Threads REQUIRED)

find_library(paho-mqttpp3 NAMES libpaho-mqttpp3.a REQUIRED)
find_library(paho-mqtt3a NAMES libpaho-mqtt3a.so REQUIRED)

add_library(pahottpp STATIC IMPORTED)

set_target_properties(pahottpp PROPERTIES IMPORTED_LOCATION ${paho-mqttpp3})

add_library(pahomqtt3a SHARED IMPORTED)
set_property(TARGET pahomqtt3a PROPERTY IMPORTED_LOCATION ${paho-mqtt3a})

# Atlas client files
file(GLOB SOURCES  "*.c" "../logger/*.c" "../commands/*.c" )

#set(CMAKE_C_FLAGS "$(CMAKE_C_FLAGS)" -Wall -w -lpthread -lpaho-mqtt3c)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(${PROJECT_NAME} paho-mqtt3c)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
