# Copyright: (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.8)
project(speech)

find_package(YARP REQUIRED)
find_package(ICUBcontrib REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH}
                              ${ICUBCONTRIB_MODULE_PATH}
                              ${CMAKE_CURRENT_SOURCE_DIR})
                              
include(YarpInstallationHelpers)
include(ICUBcontribHelpers)
include(ICUBcontribOptions)
icubcontrib_set_default_prefix()

find_package(SpeechSDK REQUIRED)
add_subdirectory(speechRecognizer)
add_subdirectory(windowsTTS)

option(BUILD_ACAPELACMD "enable the build of acapela wrapper" ON)
if(BUILD_ACAPELACMD)
    find_package(AcapelaSDK REQUIRED)
    add_subdirectory(acapelacmd)
endif()

icubcontrib_add_uninstall_target()
