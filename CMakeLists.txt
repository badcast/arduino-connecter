cmake_minimum_required(VERSION 3.0)

project(ArduinoConnector LANGUAGES CXX)

add_executable(ArduinoConnector  connector.h  general-code.hpp connector.cpp main.cpp)
