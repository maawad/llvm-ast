cmake_minimum_required(VERSION 3.12)
project(ast_example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM and Clang
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

# Set include directories
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${Clang_INCLUDE_DIRS})

# Set LLVM definitions
add_definitions(${LLVM_DEFINITIONS})

# Set the source files
set(SOURCES ast_example.cpp)

# Create the executable
add_executable(ast_example ${SOURCES})

target_compile_options(ast_example PRIVATE -fno-rtti)

# Link against LLVM and Clang libraries
target_link_libraries(ast_example PRIVATE
-Wl,-Bstatic
    clangFrontend
    clangSerialization
    clangDriver
    clangCodeGen
    clangSema
    clangAnalysis
    clangRewrite
    clangAST
    clangParse
    clangLex
    clangBasic
    LLVMMC
    LLVMCore
    LLVMSupport
    clangTooling
    clangASTMatchers
    clangAST
    clangBasic
    clangToolingCore
    LLVMCore
    LLVMSupport
    LLVMBitstreamReader
    LLVMIRReader
-Wl,-Bdynamic
)