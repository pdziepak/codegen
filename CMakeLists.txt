#
# Copyright © 2019 Paweł Dziepak
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.12)

project(codegen
  LANGUAGES CXX
)

include(CTest)

option(CODEGEN_SANTIZIERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer." ON)

list(APPEND CODEGEN_CXX_FLAGS -Wall -Wextra -Werror)
if (CODEGEN_SANTIZIERS)
  list(APPEND CODEGEN_CXX_DEBUG_FLAGS -fsanitize=address -fsanitize=address)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  list(APPEND CODEGEN_CXX_FLAGS ${CODEGEN_CXX_DEBUG_FLAGS})
else()
  list(APPEND CODEGEN_CXX_FLAGS ${CODEGEN_CXX_RELEASE_FLAGS})
endif()

find_package(LLVM 8 REQUIRED)
find_package(fmt REQUIRED)

add_library(codegen
  src/compiler.cpp
  src/module_builder.cpp
)
target_include_directories(codegen PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)
target_compile_features(codegen PUBLIC cxx_std_17)
target_link_options(codegen PRIVATE ${CODEGEN_CXX_FLAGS})
target_compile_options(codegen PRIVATE ${CODEGEN_CXX_FLAGS})
target_link_libraries(codegen PUBLIC LLVM fmt)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
