set(UDPH_CMAKE_VERSION master CACHE STRING "Git tag or branch for udphCMake.")
set(UDPH_CMAKE_NAME "updhCMake_${UDPH_CMAKE_VERSION}")

include(FetchContent)
FetchContent_Declare(
	"${UDPH_CMAKE_NAME}"
	GIT_REPOSITORY "https://github.com/udPhish/udphCMake.git"
	GIT_TAG "${UDPH_CMAKE_VERSION}"
)
string(TOLOWER "${UDPH_CMAKE_NAME}" lcName)
if(NOT ${lcName}_POPULATED)
	FetchContent_Populate(${lcName})
	set(UDPH_CMAKE_DIR "${${lcName}_SOURCE_DIR}")
endif()

include("${UDPH_CMAKE_DIR}/udphInit.cmake")