# Copyright: (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.8)
set(PROJECTNAME speechRecognizer)

file(GLOB ini ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)
file(GLOB grxml ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.grxml)

yarp_install(FILES ${ini} ${grxml} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME})
