# Copyright (C) 2014 iCub Facility
# Authors: Ali Paikan
# CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT

cmake_minimum_required(VERSION 2.8.9)
project(speech)

find_package(YARP REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH})

include_directories(${YARP_INCLUDE_DIRS})
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

include(YarpIDL)
include(YarpPlugin)
include(YarpInstallationHelpers)

set(YARP_FORCE_DYNAMIC_PLUGINS TRUE CACHE INTERNAL "${PROJECT_NAME} is always built with dynamic plugins")
yarp_configure_external_installation(${PROJECT_NAME} WITH_PLUGINS)

yarp_prepare_device(${PROJECT_NAME} 
                    TYPE yarp::dev::Speech 
                    INCLUDE speech.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${YARP_INCLUDE_DIRS}
                    ./svoxpico)

file(GLOB svox_source svoxpico/*.c)
file(GLOB svox_header svoxpico/*.h)

yarp_add_idl(IDL_GEN_FILES speech.thrift)

yarp_add_plugin(svox_speech ${svox_source}
                            ${svox_header}
                            speech.h
                            speech.cpp
                            ${IDL_GEN_FILES})

target_link_libraries(svox_speech ${YARP_LIBRARIES})

yarp_install(TARGETS svox_speech
             EXPORT YARP
             COMPONENT runtime
             LIBRARY DESTINATION ${SPEECH_DYNAMIC_PLUGINS_INSTALL_DIR}
             ARCHIVE DESTINATION ${SPEECH_STATIC_PLUGINS_INSTALL_DIR})

yarp_install(FILES svox_speech.ini
             DESTINATION ${SPEECH_PLUGIN_MANIFESTS_INSTALL_DIR})

file(GLOB lang ${CMAKE_CURRENT_SOURCE_DIR}/tts-lang/*.bin)
yarp_install(FILES ${lang}
             DESTINATION ${SPEECH_CONTEXTS_INSTALL_DIR}/${PROJECT_NAME})


