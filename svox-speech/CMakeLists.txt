# Copyright (C) 2014 iCub Facility
# Authors: Ali Paikan
# CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT

cmake_minimum_required(VERSION 2.8.9)

find_package(YARP REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH})

include_directories(${YARP_INCLUDE_DIRS})
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

include(YarpPlugin)
include(YarpInstallationHelpers)

set(YARP_FORCE_DYNAMIC_PLUGINS TRUE CACHE INTERNAL "speech is always built with dynamic plugins")
yarp_configure_external_installation(yarp)

yarp_prepare_device(speech 
                    TYPE yarp::dev::Speech 
                    INCLUDE speech.h)

#compile definition file to generate source code into the desired directory
set(IDL_GENERATED_CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${YARP_INCLUDE_DIRS}
                    ${IDL_GENERATED_CODE_DIR}/include
                    ./svoxpico)

yarp_idl_to_dir(speech.thrift ${IDL_GENERATED_CODE_DIR})


file(GLOB svox_source svoxpico/*.c)
file(GLOB svox_header svoxpico/*.h)
source_group("Source Files" FILES ${source})
source_group("Header Files" FILES ${header})

yarp_add_plugin(speech ${svox_source}
                       ${svox_header}
                       speech.h
                       speech.cpp
                       ${IDL_GENERATED_CODE_DIR}/include/Speech_IDL.h
                       ${IDL_GENERATED_CODE_DIR}/src/Speech_IDL.cpp)

target_link_libraries(speech ${YARP_LIBRARIES})

yarp_install(TARGETS speech
             EXPORT YARP
             COMPONENT runtime
             LIBRARY DESTINATION ${YARP_DYNAMIC_PLUGINS_INSTALL_DIR}
             ARCHIVE DESTINATION ${YARP_STATIC_PLUGINS_INSTALL_DIR})

yarp_install(FILES speech.ini
             DESTINATION ${YARP_PLUGIN_MANIFESTS_INSTALL_DIR})


