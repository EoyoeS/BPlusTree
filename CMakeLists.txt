cmake_minimum_required(VERSION 3.16)
project(BPlusTree LANGUAGES CXX)
include_directories(include)
add_executable(${PROJECT_NAME} test/main.cpp src/bplus_tree.cpp src/serialize.cpp)