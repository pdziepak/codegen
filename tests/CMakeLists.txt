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

find_package(GTest REQUIRED)

function(codegen_add_test TESTNAME SOURCE)
  add_executable(${TESTNAME} ${SOURCE} ${ARGN})
  target_link_libraries(${TESTNAME} codegen GTest::GTest GTest::Main)
  target_link_options(${TESTNAME} PRIVATE ${CODEGEN_CXX_FLAGS})
  target_compile_options(${TESTNAME} PRIVATE ${CODEGEN_CXX_FLAGS})
  add_test(${TESTNAME} ${TESTNAME})
endfunction(codegen_add_test)

codegen_add_test(arithmetic_ops arithmetic_ops.cpp)
codegen_add_test(module_builder module_builder.cpp)
codegen_add_test(relational_ops relational_ops.cpp)
codegen_add_test(statements statements.cpp)
codegen_add_test(variable variable.cpp)
