# Copyright 2017 MongoDB Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Demonstrates how to use the CMake 'find_package' mechanism to locate build against the libmongocxx
# shared library.

cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

if(POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif()

project(POLARIMETER LANGUAGES C CXX)

# Enforce the C++ standard, and disable extensions.
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions("-std=c++11 -march=native -mtune=native")

set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
endif()

# NOTE: For this to work, the CMAKE_PREFIX_PATH variable must be set to point to the directory that
# was used as the argument to CMAKE_INSTALL_PREFIX when building libmongocxx.
find_package(libmongocxx REQUIRED)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hpp)
add_executable(polarimeter ./main.cpp  ${sources} ${headers})

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    find_package(Boost 1.56.0 REQUIRED)
    target_include_directories(polarimeter
      PRIVATE ${Boost_INCLUDE_DIRS}
    )
endif()

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{ROOTSYS}/ect/cmake")
find_package(ROOT REQUIRED COMPONENTS RIO Net)
include(${ROOT_USE_FILE})

message(${ROOT_LIBRARY_DIR})
message(${ROOT_LIBRARIES})

# include(FindROOT.cmake)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${ROOT_INCLUDE_DIR})
link_directories(${ROOT_LIBRARY_DIR})

target_include_directories(polarimeter
  PRIVATE ${LIBMONGOCXX_INCLUDE_DIRS}
)

target_link_libraries(polarimeter
  PRIVATE ${LIBMONGOCXX_LIBRARIES};
  ${ROOT_LIBRARIES}
  CAENDigitizer
  ${ROOT_LIBRARY_DIR}/libRHTTP.so # for THttp
  ${ROOT_LIBRARY_DIR}/libSpectrum.so # for TSpectrum
)

target_compile_definitions(polarimeter
  PRIVATE ${LIBMONGOCXX_DEFINITIONS}
)

add_custom_target(run
    COMMAND polarimeter
    DEPENDS polarimeter
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)

# Sanity-check that static library macros are not set when building against the shared library.
# Users don't need to include this section in their projects.
list(FIND LIBMONGOCXX_DEFINITIONS "BSONCXX_STATIC" LIST_IDX)
if (${LIST_IDX} GREATER -1)
    message(FATAL_ERROR "Expected BSONCXX_STATIC to not be defined")
endif()
list(FIND LIBMONGOCXX_DEFINITIONS "MONGOCXX_STATIC" LIST_IDX)
if (${LIST_IDX} GREATER -1)
    message(FATAL_ERROR "Expected MONGOCXX_STATIC to not be defined")
endif()
