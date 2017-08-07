cmake_minimum_required(VERSION 2.8.11)
project(BA)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
# find_package(ZLIB REQUIRED)


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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# file(GLOB ba_SRC "*.h" "*.cc")

add_executable(ba main.cc assert.cc inspect/glwidget.cc inspect/mainwindow.cc inspect/window.cc)
# add_executable(plycat utils/plycat.cc)

qt5_use_modules(ba Widgets OpenGL)

# Use the Widgets module from Qt 5.
target_link_libraries(ba ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES})

include_directories("${CMAKE_SOURCE_DIR}/" "${CMAKE_SOURCE_DIR}/inspect" ${QT_INCLUDES} ${GLUT_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

