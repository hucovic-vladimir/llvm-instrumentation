cmake_minimum_required(VERSION 3.13.4)

# add_library(FunctionRuntime SHARED FunctionRuntime.cpp)
# add_library(GetDebugInfo SHARED GetDebugInfo.cpp)
# add_library(LoopPass SHARED LoopPass.cpp)
# add_library(ParamType SHARED ParamType.cpp)
# add_library(BasicBlockInst SHARED BasicBlockInst.cpp PassUtilities.cpp)
add_library(InstructionCount SHARED InstructionCount.cpp PassUtilities.cpp InstrumentationFunctions.cpp)
add_library(PostInstrumentationPass SHARED PostInstrumentationPass.cpp)
# add_executable(instrument Instrumentation.cpp BBAnalysisPass.cpp BBInstrumentationPass.cpp PassUtilities.cpp)
# add_executable(instrument2 Instrumentation2.cpp BBAnalysisPass.cpp BBInstrumentationPass.cpp PassUtilities.cpp)
# target_link_libraries(instrument ${llvm_libs})
# target_link_libraries(instrument2 ${llvm_libs})
