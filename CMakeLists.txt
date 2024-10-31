cmake_minimum_required(VERSION 3.5)
project(duality VERSION 0.0.1)

set(DUALITY_ROOT_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(DUALITY_ROOT_PROJECT ON)
endif ()

option(DUALITY_BUILD_TESTS "Build tests for nfd" ${DUALITY_ROOT_PROJECT})
option(DUALITY_INSTALL "Generate install target for nfd" ${DUALITY_ROOT_PROJECT})

# Use latest C++ by default (should be the best one), but let user override it
if(NOT DEFINED CMAKE_CXX_STANDARD)
  if(CMAKE_VERSION VERSION_LESS "3.8")
    set (CMAKE_CXX_STANDARD 14)
  elseif(CMAKE_VERSION VERSION_LESS "3.12")
    set (CMAKE_CXX_STANDARD 17)
  elseif(CMAKE_VERSION VERSION_LESS "3.20")
    set (CMAKE_CXX_STANDARD 20)
  else()
    set (CMAKE_CXX_STANDARD 23)
  endif()
endif()

add_subdirectory(include)
if(${DUALITY_BUILD_TESTS})
  add_subdirectory(lib/Catch2)
  add_subdirectory(test)
endif()

# if(${NFD_BUILD_TESTS} OR ${NFD_BUILD_SDL2_TESTS})
#   add_subdirectory(test)
# endif()
