
IF(DEFINED $ENV{SPEECH_SDK_DIR})
	message("SPEECH_SDK_DIR is defined. Overriding the standard location.")
	SET(SpeechSDK_ROOT_DIR "$ENV{SPEECH_SDK_DIR}")
ELSE()
	SET(SpeechSDK_ROOT_DIR "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A")
ENDIF()

SET(SpeechSDK_LIB_DIR "${SpeechSDK_ROOT_DIR}/Lib/")
SET(SpeechSDK_INCLUDE_DIR "${SpeechSDK_ROOT_DIR}/Include")
SET(SpeechSDK_LIBRARIES 
	"${SpeechSDK_LIB_DIR}/sapi.lib"
)
SET(SpeechSDK_INCLUDES 
	"${SpeechSDK_INCLUDE_DIR}/sapi.h"
	"${SpeechSDK_INCLUDE_DIR}/sapiddk.h"
	"${SpeechSDK_INCLUDE_DIR}/sperror.h"
	"${SpeechSDK_INCLUDE_DIR}/sphelper.h"
)

####################   Macro   #######################
MACRO(CHECK_FILES _FILES _DIR)
	SET(_MISSING_FILES)
	FOREACH(_FILE ${${_FILES}})
		IF(NOT EXISTS "${_FILE}")
			SET(SpeechSDK_FOUND NO)
			get_filename_component(_FILE ${_FILE} NAME)
			SET(_MISSING_FILES "${_MISSING_FILES}${_FILE}, ")
		ENDIF()
	ENDFOREACH()
	IF(_MISSING_FILES)
		MESSAGE(STATUS "In folder \"${${_DIR}}\" not found files: ${_MISSING_FILES}")
		SET(SpeechSDK_FOUND NO)
	ENDIF()
ENDMACRO(CHECK_FILES)

MACRO(CHECK_DIR _DIR)
	IF(NOT EXISTS "${${_DIR}}")
		#MESSAGE(STATUS "Folder \"${${_DIR}}\" not found.")
		SET(SpeechSDK_FOUND NO)
	ENDIF()
ENDMACRO(CHECK_DIR)

##################### Checking #######################
MESSAGE(STATUS "Searching SpeechSDK.")
SET(SpeechSDK_FOUND TRUE)

CHECK_DIR(SpeechSDK_ROOT_DIR)
IF(SpeechSDK_FOUND)
	CHECK_DIR(SpeechSDK_LIB_DIR)
	CHECK_DIR(SpeechSDK_INCLUDE_DIR)
	
	IF(SpeechSDK_FOUND)
		CHECK_FILES(SpeechSDK_LIBRARIES SpeechSDK_LIB_DIR)
		CHECK_FILES(SpeechSDK_INCLUDES SpeechSDK_INCLUDE_DIR)
	ENDIF()
ENDIF()

MESSAGE(STATUS "SpeechSDK_FOUND - ${SpeechSDK_FOUND}.")