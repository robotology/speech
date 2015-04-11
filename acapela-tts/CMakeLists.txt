# Copyright: (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.8)
set(PROJECTNAME acapela-tts)
project(${PROJECTNAME})

set(sources main.cpp)
source_group("Source Files" FILES ${sources})
include_directories(${AcapelaSDK_INCLUDE_DIRS})
add_executable(${PROJECTNAME} ${sources})
install(TARGETS ${PROJECTNAME} DESTINATION bin)
