cmake_minimum_required(VERSION 3.10)
project(ipcan)

set(CMAKE_CXX_STANDARD 11)

add_executable(ipcan main.cc ActiveNode.cc ActiveNode.h Trie.cc Trie.h cpp/utils.cpp header/utils.h cpp/Experiment.cpp header/Experiment.h cpp/QueryResult.cpp header/QueryResult.h header/Directives.h)