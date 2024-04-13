cmake_minimum_required(VERSION 3.10.0)
set (CMAKE_CXX_STANDARD 17)

#project name
project (udp)

include_directories(/Library/Developer/CommandLineTools/usr/include)
include_directories(SharedClasses)

add_subdirectory(SharedClasses)

add_executable(client Client/Classes/main.cpp)

add_executable(server Server/Classes/main.cpp)
target_link_libraries(server PRIVATE ConfigManager)