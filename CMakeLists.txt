# CMAKE VERSION
cmake_minimum_required(VERSION 3.18.0)

include(cmake/udph.cmake)

project(libudph_Class VERSION "${GIT_VERSION}" LANGUAGES CXX)

include(udphInit)

PROJECT_SET_NAMESPACE(libudph_Class libudph)

TARGET_CREATE_LIB(Class INTERFACE)

TARGET_APPEND_HDR(
	Class
	"libudph/Class/Event.h"
	"libudph/Class/Interface.h"
	"libudph/Class/SFINAE.h"
	"libudph/Class/Traits.h"
)

TARGET_LINK(Class)
TARGET_PACKAGE(Class)

include(udphFinalize)