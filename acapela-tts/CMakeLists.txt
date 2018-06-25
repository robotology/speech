# Copyright: (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 3.5)
project(acapela-tts)

set(sources main.cpp)
source_group("Source Files" FILES ${sources})
include_directories(${AcapelaSDK_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} ${sources})
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
