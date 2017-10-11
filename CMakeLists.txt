cmake_minimum_required(VERSION 2.8.11)

project(Harry)

set(FMTS hry ply obj) # specifies the formats you want to compile
set(EXE_NAME harry) # specifies the name of the executable

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'RELEASE' as none was specified.")
    set(CMAKE_BUILD_TYPE RELEASE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# c++14 support
if(CMAKE_VERSION VERSION_LESS "3.1")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "--std=gnu++14 ${CMAKE_CXX_FLAGS}")
	endif()
else()
	set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w") # :)

set(FMTSRC "")
foreach(f ${FMTS})
	string(TOUPPER ${f} F)
	add_definitions(-DWITH_${F})
	LIST(APPEND FMTSRC "formats/${f}/reader.cc")
	LIST(APPEND FMTSRC "formats/${f}/writer.cc")
endforeach()

add_executable(${EXE_NAME} main.cc ${FMTSRC})
target_link_libraries(${EXE_NAME} ${CMAKE_THREAD_LIBS_INIT})
