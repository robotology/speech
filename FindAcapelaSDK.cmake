
set(AcapelaSDK_ROOT_DIR $ENV{ACAPELA_SDK_DIR} CACHE PATH "Path to Acapela SDK directory")
set(AcapelaSDK_INCLUDE_DIRS ${AcapelaSDK_ROOT_DIR}/include)

if(EXISTS ${AcapelaSDK_INCLUDE_DIRS})    
    set(AcapelaSDK_FOUND TRUE)
else()
    set(AcapelaSDK_FOUND FALSE)
endif()

messagse(STATUS "AcapelaSDK - ${AcapelaSDK_FOUND}.")
