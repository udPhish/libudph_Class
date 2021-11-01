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

TARGET_CREATE_EXE(Test_Class)
TARGET_SET_DIR(Test_Class "libudph/Test/Class")
TARGET_APPEND_SRC(
	Test_Class
	"main.cpp"
)
TARGET_APPEND_DEP(Test_Class libudph::Class)

TARGET_LINK(Test_Class)
TARGET_PACKAGE(Test_Class)

FINALIZE()