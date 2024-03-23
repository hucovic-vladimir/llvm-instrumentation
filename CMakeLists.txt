# TODO improve this configuration
# also make proper documentation

set(CMAKE_BUILD_TYPE Debug)
cmake_minimum_required(VERSION 3.13.4)
project(FunctionRuntimeLLVMPass)

message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(LLVM PATHS "~/llvm-debug/build/lib/cmake/llvm" NO_DEFAULT_PATH REQUIRED CONFIG)

message(STATUS "LLVM Include dirs: ${LLVM_INCLUDE_DIRS}")

# set(LLVM_INSTALL_DIR "~/llvm-debug/build/" CACHE PATH "LLVM installation directory")

# set(LLVM_INCLUDE_DIRS "${LLVM_INSTALL_DIR}/include/llvm/")

# set(LLVM_TOOLS_BINARY_DIR "${LLVM_INSTALL_DIR}/bin/")

# list(APPEND CMAKE_PREFIX_PATH "${LLVM_INSTALL_DIR}/lib/cmake/llvm/")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
# link_directories(${LLVM_LIBRARY_DIRS})
# add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 20 CACHE STRING "")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

llvm_map_components_to_libnames(llvm_libs support core irreader executionengine native passes bitwriter)

if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

add_subdirectory(src)
