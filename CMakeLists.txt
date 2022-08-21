cmake_minimum_required(VERSION 3.0)

project(ArduinoConnector LANGUAGES CXX)

#DAEMON
add_executable(connectord connector.h general-code.hpp connector.cpp connector-pipe.h conectord.cpp)

#CLIENT
add_executable(connector connector-pipe.h main.cpp)
