cmake_minimum_required(VERSION 2.8)

project(data_plane)

find_package(Threads REQUIRED)

# Atlas client files
file(GLOB SOURCES  "*.c" "../logger/*.c" "../commands/*.c" )


add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(${PROJECT_NAME} paho-mqtt3c)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
