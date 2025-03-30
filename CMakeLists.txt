set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.1" CACHE STRING "Minimum OS X deployment version")
cmake_minimum_required(VERSION 3.13.4)
project(llvm_instrumentation)

find_package(LLVM PATHS ${LLVM_CMAKE_PATH} NO_DEFAULT_PATH REQUIRED CONFIG)

message(STATUS "LLVM Include dirs: ${LLVM_INCLUDE_DIRS}")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 20 CACHE STRING "")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

llvm_map_components_to_libnames(llvm_libs support core irreader executionengine native passes bitwriter)

if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

add_subdirectory(src)
