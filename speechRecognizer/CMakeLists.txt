cmake_minimum_required(VERSION 2.6)
include(YarpInstallationHelpers)

find_package(ICUBcontrib)
list(APPEND CMAKE_MODULE_PATH ${ICUBCONTRIB_MODULE_PATH})
include(ICUBcontribHelpers)
icubcontrib_set_default_prefix()

add_subdirectory(src)
add_subdirectory(app)
