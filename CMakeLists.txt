cmake_minimum_required(VERSION 2.8)

project(atlas_client)

include(FindPkgConfig)
# CoAP dependency
pkg_check_modules(COAP libcoap-2-openssl REQUIRED)

include_directories(include)
# Add CoAP headers
include_directories(${COAP_INCLUDE_DIRS})

# Atlas client files
file(GLOB SOURCES "src/*.c" "src/coap_server/*.c" "src/logger/*.c" "src/scheduler/*.c" "src/alarm/*.c")

link_directories(${COAP_LIBRARY_DIRS})
add_executable(${PROJECT_NAME} ${SOURCES})

#Add CoAP library
target_link_libraries(${PROJECT_NAME} ${COAP_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
