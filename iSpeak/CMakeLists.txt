# Copyright: (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 3.5)
set(PROJECTNAME iSpeak)
project(${PROJECTNAME})

set(sources main.cpp)
source_group("Source Files" FILES ${sources})
add_executable(${PROJECTNAME} ${sources})
target_link_libraries(${PROJECTNAME} ${YARP_LIBRARIES})
install(TARGETS ${PROJECTNAME} DESTINATION bin)

