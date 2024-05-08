cmake_minimum_required(VERSION 3.13.4)

add_library(InstructionCount SHARED InstructionCount.cpp PassUtilities.cpp InstrumentationFunctions.cpp)
add_library(PostInstrumentationPass SHARED PostInstrumentationPass.cpp)
