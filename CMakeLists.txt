cmake_minimum_required(VERSION 3.3.2)
project(aDTN-lite CXX)

execute_process(COMMAND /usr/bin/llvm-config --cxxflags OUTPUT_VARIABLE LLVM_CONFIG OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CONFIG}")
#set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
#set(CMAKE_C_COMPILER "/usr/bin/clang")

### Options
option(build_tests "Build tests" ON)
include_directories(${CMAKE_SOURCE_DIR})

### Find libraries
### Build dependencies
# Build libubpf
add_custom_target(libubpf COMMAND make WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vendor/ubpf/vm/)
# Build xxHash
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/xxHash/cmake_unofficial)
include_directories(vendor/xxHash)
# Clang/LLVM Backend 
# include_directories(/usr/local/llvm-debug/include/)
# link_directories(/usr/local/llvm-debug/lib/)
execute_process(COMMAND /usr/bin/llvm-config --libs OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND /usr/bin/llvm-config --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CLANG_LIBS clang clangFrontend clangDriver clangSerialization clangParse
    clangCodeGen  clangSema clangAnalysis clangEdit clangAST clangLex
    clangBasic )
set(EXECUTOR_LIBRARIES ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS} ${CLANG_LIBS}
    ${CMAKE_SOURCE_DIR}/vendor/ubpf/vm/libubpf.a
    ${CMAKE_BINARY_DIR}/vendor/xxHash/cmake_unofficial/libxxhash.so) 

### aDTN-lite modules
# Misc
set(MISC Utils/Logger.cpp Utils/Logger.h Utils/Logstream.cpp Utils/Logstream.h)
# Executor
include_directories(${CMAKE_SOURCE_DIR}/Executor
    ${CMAKE_SOURCE_DIR}/Executor/Backend} ${CMAKE_SOURCE_DIR}/vendor/ubpf)
set(EXECUTOR Executor/Backend/Backend.h Executor/Backend/ClangBackend.h Executor/Backend/ClangBackend.cpp)

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

    # TODO: Downlaod google test only if build_tests is enabled
    # Compile google-test
    add_subdirectory(vendor/googletest)
    include_directories(${gtest_SOURCE_DIR}/include)

    add_executable(ExecutorTest Executor/tests/ExecutorTest.cpp
        ${EXECUTOR} ${MISC})          
    target_link_libraries(ExecutorTest ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS}
        ${CLANG_LIBS} ${EXECUTOR_LIBRARIES} gtest magic)
    add_dependencies(ExecutorTest xxhash)
endif()
