cmake_minimum_required(VERSION 3.3.2)

### Set clang as compiler
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")

### Define project and make the compiler use c++14
project(aDTN-lite CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 ")

### Options
option(build_tests "Build tests" ON)
option(perf_logs "Build with performance logging" ON)
if (perf_logs)
    add_definitions(-DPERF)
endif()

include_directories(${CMAKE_SOURCE_DIR})

### Build dependencies

# Build libubpf
add_custom_target(libubpf COMMAND make WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vendor/ubpf/vm/)

# Build xxHash
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/xxHash/cmake_unofficial)
include_directories(vendor/xxHash)

# Clang/LLVM Backend 
# include_directories(/usr/local/llvm-debug/include/)
# link_directories(/usr/local/llvm-debug/lib/)
execute_process(COMMAND /usr/bin/llvm-config --libs OUTPUT_VARIABLE LLVM_LIBS 
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND /usr/bin/llvm-config --cxxflags OUTPUT_VARIABLE
    LLVM_CONFIG OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND /usr/bin/llvm-config --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CLANG_LIBS clang clangFrontend clangDriver clangSerialization clangParse
    clangCodeGen  clangSema clangAnalysis clangEdit clangAST clangLex
    clangBasic )

### aDTN-lite modules

# Misc
set(MISC Utils/Logger.cpp Utils/Logger.h Utils/Logstream.cpp Utils/Logstream.h)

# Executor
include_directories(${CMAKE_SOURCE_DIR}/Executor
    ${CMAKE_SOURCE_DIR}/Executor/CompilerBackend}
    ${CMAKE_SOURCE_DIR}/vendor/ubpf ${CMAKE_SOURCE_DIR}/vendor/ubpf/vm/inc)
set(EXECUTOR Executor/CompilerBackend/CompilerBackend.h
    Executor/CompilerBackend/ClangCompilerBackend.h
    Executor/CompilerBackend/ClangCompilerBackend.cpp
    Executor/ExecBackend/eBPFExecBackend.h
    #     Executor/ExecBackend/eBPFExecBackend.cpp
    Executor/Code.h)
set(EXECUTOR_LIBRARIES ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS} ${CLANG_LIBS}
    ${CMAKE_SOURCE_DIR}/vendor/ubpf/vm/libubpf.a
    ${CMAKE_BINARY_DIR}/vendor/xxHash/cmake_unofficial/libxxhash.so
    papi) 
execute_process(COMMAND /usr/bin/llvm-config --cxxflags OUTPUT_VARIABLE LLVM_CONFIG OUTPUT_STRIP_TRAILING_WHITESPACE)
set_source_files_properties(Executor/CompilerBackend/ClangCompilerBackend.h
    Executor/CompilerBackend/ClangCompilerBackend.cpp PROPERTIES COMPILE_FLAGS
    ${LLVM_CONFIG})

# AppManager
include_directories(${CMAKE_SOURCE_DIR}/AppManager)
set(APPMANAGER AppManager/Application.h AppManager/Application.cpp
    AppManager/AppManager.h AppManager/AppManager.cpp)

### aDTN-lite
set(SOURCE_FILES ${MISC} ${EXECUTOR} ${APPMANAGER})
add_executable(aDTN-lite main.cpp ${SOURCE_FILES})
target_link_libraries(aDTN-lite ${EXECUTOR_LIBRARIES})

### Dependencies (build them befor aDTN-lite)
add_dependencies(aDTN-lite libubpf)
add_dependencies(aDTN-lite xxhash)

### Tests
if (build_tests)
    enable_testing()

    add_subdirectory(vendor/googletest/googletest)
    include_directories(${gtest_SOURCE_DIR}/include)

    add_executable(ExecutorTest Executor/tests/ExecutorTest.cpp
        ${EXECUTOR} ${MISC})          
    target_link_libraries(ExecutorTest ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS}
        ${CLANG_LIBS} ${EXECUTOR_LIBRARIES} gtest magic bz2)
    add_dependencies(ExecutorTest xxhash)
endif()
