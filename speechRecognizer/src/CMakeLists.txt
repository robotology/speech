# Copyright: (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 3.5)
project(speechRecognizer)
if(MSVC12)
	message("Visual Studio 2013 detected. Forcing backcompatibility.")
	add_definitions(-D_USING_V110_SDK71_)
endif(MSVC12)

file(GLOB folder_source *.h *.cpp)
source_group("Source Files" FILES ${folder_source})

include_directories(${SpeechSDK_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} ${folder_source})
target_link_libraries(${PROJECT_NAME} ${YARP_LIBRARIES} ${SpeechSDK_LIBRARIES})
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
