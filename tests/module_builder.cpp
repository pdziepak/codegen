/*
 * Copyright © 2019 Paweł Dziepak
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <gtest/gtest.h>

#include "codegen/compiler.hpp"
#include "codegen/module.hpp"
#include "codegen/module_builder.hpp"

TEST(module_builder, empty) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "empty");
  auto module = std::move(builder).build();
  (void)module;
}

TEST(module_builder, return_void) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "return_void");
  auto fn = builder.create_function<void()>("return_void_fn", [] {
    codegen::return_();
  });
  auto module = std::move(builder).build();
  auto fn_ptr = module.get_address(fn);
  fn_ptr();
}
