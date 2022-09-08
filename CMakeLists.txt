cmake_minimum_required(VERSION 3.0)

project(ArduinoConnector LANGUAGES CXX)

set(CLIENT_NAME "aconnector")
set(DAEMON_NAME "${CLIENT_NAME}d")

#set global define
add_definitions(-DDAEMON_NAME="${DAEMON_NAME}" -DCLIENT_NAME="${CLIENT_NAME}")

#DAEMON
add_executable(${DAEMON_NAME} connector.h general-code.hpp connector.cpp connector-pipe.h conectord.cpp)

#CLIENT
add_executable(${CLIENT_NAME} connector-pipe.h main.cpp)
