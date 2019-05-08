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

#include "codegen/variable.hpp"

#include <gtest/gtest.h>

#include "codegen/arithmetic_ops.hpp"
#include "codegen/compiler.hpp"
#include "codegen/module.hpp"
#include "codegen/module_builder.hpp"

TEST(variable, set_get) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "set_get");

  auto set_get = builder.create_function<int32_t(int32_t)>("set_get_fn", [](codegen::value<int32_t> x) {
    auto y = codegen::variable<int32_t>{"y"};
    y.set(x);
    auto z = codegen::variable<int32_t>{"z"};
    auto expr = y.get() + codegen::constant<int32_t>(1);
    y.set(codegen::constant<int32_t>(4));
    z.set(expr);
    codegen::return_(y.get() + z.get());
  });

  auto module = std::move(builder).build();

  auto set_get_ptr = module.get_address(set_get);
  EXPECT_EQ(set_get_ptr(8), 13);
}
