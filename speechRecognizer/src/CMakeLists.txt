# Copyright: (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.8)
set(PROJECTNAME speechRecognizer)
project(${PROJECTNAME})

file(GLOB folder_source *.h *.cpp)
source_group("Source Files" FILES ${folder_source})

include_directories(${YARP_INCLUDE_DIRS} ${SpeechSDK_INCLUDE_DIRS})
add_executable(${PROJECTNAME} ${folder_source})
target_link_libraries(${PROJECTNAME} ${YARP_LIBRARIES} ${SpeechSDK_LIBRARIES})
install(TARGETS ${PROJECTNAME} DESTINATION bin)
