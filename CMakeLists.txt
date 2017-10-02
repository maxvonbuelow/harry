cmake_minimum_required(VERSION 2.8.11)

project(Harry)

set(FMTS hry ply obj)
set(EXE_NAME harry)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if (CMAKE_VERSION VERSION_LESS "3.1")
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set (CMAKE_CXX_FLAGS "--std=gnu++14 ${CMAKE_CXX_FLAGS}")
	endif ()
else ()
	set (CMAKE_CXX_STANDARD 14)
endif ()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

find_package(Threads REQUIRED)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w") # :)

set(FMTSRC "")
FOREACH(f ${FMTS})
	LIST(APPEND FMTSRC "formats/${f}/reader.cc")
	LIST(APPEND FMTSRC "formats/${f}/writer.cc")
ENDFOREACH(f)

add_executable(${EXE_NAME} main.cc assert.cc ${FMTSRC})

include_directories("${CMAKE_SOURCE_DIR}/")

