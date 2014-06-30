# Copyright: (C) 2011 EFAA Consortium
# Authors: Stephane lallee
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.6)
set(PROJECTNAME speechRecognizer)
project(${PROJECTNAME})

file(GLOB folder_source *.h *.cpp)
source_group("Source Files" FILES ${folder_source})

include_directories(${YARP_INCLUDE_DIRS} ${SpeechSDK_INCLUDE_DIR})
add_executable(${PROJECTNAME} ${folder_source})
target_link_libraries(${PROJECTNAME} ${YARP_LIBRARIES} ${SpeechSDK_LIBRARIES})
install(TARGETS ${PROJECTNAME} DESTINATION bin)

