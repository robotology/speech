cmake_minimum_required(VERSION 2.6)
project(speech)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

find_package(YARP)
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH})

find_package(SpeechSDK)
if(SpeechSDK_FOUND)
   add_subdirectory(speechRecognizer)
endif(SpeechSDK_FOUND)