cmake_minimum_required(VERSION 3.12)
project(cloud-drive VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig)

pkg_check_modules(HIREDIS REQUIRED hiredis)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(UUID REQUIRED uuid)

# bcrypt - use glibc crypt_r (linked via -lcrypt)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/controllers
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/services
    ${CMAKE_SOURCE_DIR}/middlewares
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/plugins
    ${CMAKE_SOURCE_DIR}/tasks
    ${DROGON_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Source files
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/controllers/*.cpp
    ${CMAKE_SOURCE_DIR}/models/*.cpp
    ${CMAKE_SOURCE_DIR}/services/*.cpp
    ${CMAKE_SOURCE_DIR}/middlewares/*.cpp
    ${CMAKE_SOURCE_DIR}/utils/*.cpp
    ${CMAKE_SOURCE_DIR}/plugins/*.cpp
    ${CMAKE_SOURCE_DIR}/tasks/*.cpp
    ${CMAKE_SOURCE_DIR}/main.cpp
)

# Main executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    Drogon::Drogon
    ${HIREDIS_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${UUID_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
    crypt
)

# Copy config to build directory
configure_file(
    ${CMAKE_SOURCE_DIR}/config/config.json
    ${CMAKE_BINARY_DIR}/config.json COPYONLY
)
