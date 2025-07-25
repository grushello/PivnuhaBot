cmake_minimum_required(VERSION 3.10.2)
project(pivnuhaBot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(Boost_USE_MULTITHREADED ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(CURL)
find_package(xlnt CONFIG REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/include)
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()

add_executable(pivnuhaBot src/main.cpp src/time.cpp src/user.cpp src/telegram_handler.cpp src/table_handler.cpp src/user_handler.cpp)

target_link_libraries(pivnuhaBot 
    ${CMAKE_THREAD_LIBS_INIT} 
    ${OPENSSL_LIBRARIES} 
    ${Boost_LIBRARIES} 
    ${CURL_LIBRARIES}
    ${CMAKE_SOURCE_DIR}/external/vcpkg/installed/x64-windows/lib/TgBot.lib
    xlnt::xlnt)