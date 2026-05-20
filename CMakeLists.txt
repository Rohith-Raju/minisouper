cmake_minimum_required(VERSION 3.13)
project(MiniSouper)

# Enable debug symbols
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")

# Use clang from LLVM build
set(CMAKE_C_COMPILER "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/llvm-Release-install/bin/clang")
set(CMAKE_CXX_COMPILER "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/llvm-Release-install/bin/clang++")

# Use LLVM from Souper's third_party
set(LLVM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/llvm-Release-install/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Add Z3 from Souper's third_party
set(Z3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/z3-install/include")
set(Z3_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/z3-install/lib64/libz3.so")
include_directories(${Z3_INCLUDE_DIR})

# Mini-Souper: Educational superoptimizer
add_executable(mini_souper
  src/main.cpp
  src/Expr.cpp
  src/Z3Solver.cpp
)

target_compile_options(mini_souper PRIVATE -g -ggdb3)

llvm_map_components_to_libnames(llvm_libs support core irreader analysis)
target_link_libraries(mini_souper ${llvm_libs} ${Z3_LIBRARY} stdc++)

set_property(TARGET mini_souper PROPERTY CXX_STANDARD 17)
