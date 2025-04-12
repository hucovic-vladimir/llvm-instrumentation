cmake_minimum_required(VERSION 3.13.4)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_library(InstructionCount SHARED InstructionCount.cpp PassUtilities.cpp InstrumentationFunctions.cpp)
add_library(PostInstrumentationPass SHARED PostInstrumentationPass.cpp)
add_library(PathInstrumentation SHARED PathInstrumentation.cpp lib/CFGTransformer.cpp)

target_link_libraries(InstructionCount ${llvm_libs})
target_link_libraries(PostInstrumentationPass ${llvm_libs})
target_link_libraries(PathInstrumentation ${llvm_libs})





