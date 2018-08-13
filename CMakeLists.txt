cmake_minimum_required(VERSION 3.11)
set (CMAKE_CXX_STANDARD 17)

find_package(LLVM)

include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_BINARY_DIRS})

add_subdirectory(BBCountPass)

set_target_properties(CountBBPass PROPERTIES COMPILE_FLAGS "-D__GLIBCXX_USE_CXX11_ABI=0 -fno-rtti" CXX_STANDARD 17)
