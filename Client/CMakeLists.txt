cmake_minimum_required(VERSION 3.10.0)
set (CMAKE_CXX_STANDARD 11)

#project name
project (client)

include_directories(/Library/Developer/CommandLineTools/usr/include)

add_executable(client Classes/main.cpp)