cmake_minimum_required(VERSION 3.13.4)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})


file(GLOB LIB_SOURCES lib/*.cpp)
add_library(InstrLibs STATIC ${LIB_SOURCES} InstrumentationFunctions.cpp)
target_link_libraries(InstrLibs ${llvm_libs})


add_library(InstructionCount SHARED InstructionCount.cpp PassUtilities.cpp InstrumentationFunctions.cpp)
add_library(PostInstrumentationPass SHARED PostInstrumentationPass.cpp)
add_library(PathInstrumentation SHARED PathInstrumentation.cpp)

target_link_libraries(InstructionCount InstrLibs ${llvm_libs})
target_link_libraries(PostInstrumentationPass InstrLibs ${llvm_libs})
target_link_libraries(PathInstrumentation InstrLibs ${llvm_libs})





