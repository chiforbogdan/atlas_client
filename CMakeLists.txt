cmake_minimum_required(VERSION 2.8)

project(atlas_client)

include(FindPkgConfig)

# CoAP dependency
pkg_check_modules(COAP libcoap-2-openssl REQUIRED)

# UUID dependency
pkg_check_modules(UUID uuid REQUIRED)

# OpenSSL dependency
find_package(OpenSSL REQUIRED)

include_directories(include)
# Add CoAP headers
include_directories(${COAP_INCLUDE_DIRS})

# Atlas client files
file(GLOB SOURCES "src/*.c" "src/coap/*.c" "src/logger/*.c" "src/scheduler/*.c" "src/alarm/*.c", "src/identity/*.c" "src/commands/*.c" "src/register/*.c" "src/telemetry/*.c" "src/utils/*.c" "src/received_commands/*.c")

link_directories(${COAP_LIBRARY_DIRS})
add_executable(${PROJECT_NAME} ${SOURCES})

#Add CoAP and UUID library
target_link_libraries(${PROJECT_NAME} ${COAP_LIBRARIES} ${UUID_LIBRARIES} OpenSSL::Crypto)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
