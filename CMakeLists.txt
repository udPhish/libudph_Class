# CMAKE VERSION
cmake_minimum_required(VERSION 3.18.0)

include(cmake/udph.cmake)

project_create(libudph_Class)

PROJECT_SET_NAMESPACE(libudph_Class libudph)

TARGET_CREATE_LIB(Class STATIC)
TARGET_APPEND_SRC(
	Class
	"libudph/Class/dummy.cpp"
)
TARGET_APPEND_HDR(
	Class
	"libudph/Class/Event.h"
	"libudph/Class/Interface.h"
	"libudph/Class/SFINAE.h"
	"libudph/Class/Traits.h"
)

TARGET_LINK(Class)

TARGET_PACKAGE(Class)

FINALIZE()