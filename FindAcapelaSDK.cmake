
set(AcapelaSDK_ROOT_DIR "C:/Program Files (x86)/Acapela Group/Acapela TTS for Windows/sdk"
    CACHE PATH "Path to Acapela SDK directory")
set(AcapelaSDK_INCLUDE_DIRS ${AcapelaSDK_ROOT_DIR}/include)

if(EXISTS ${AcapelaSDK_INCLUDE_DIRS})    
    set(AcapelaSDK_FOUND TRUE)
else()
    set(AcapelaSDK_FOUND FALSE)
endif()

message(STATUS "AcapelaSDK - ${AcapelaSDK_FOUND}.")
