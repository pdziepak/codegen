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

#include "codegen/statements.hpp"

#include <gtest/gtest.h>

#include "codegen/arithmetic_ops.hpp"
#include "codegen/compiler.hpp"
#include "codegen/module.hpp"
#include "codegen/module_builder.hpp"
#include "codegen/relational_ops.hpp"
#include "codegen/variable.hpp"

TEST(statements, if_condition) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "if_cond");

  auto if_cond = builder.create_function<int32_t(int32_t)>("if_cond_fn", [](codegen::value<int32_t> x) {
    auto y = codegen::variable<int32_t>{"ret"};
    codegen::if_(
        x > codegen::constant<int32_t>(4), [&] { y.set(x + x); }, [&] { y.set(x * x); });
    codegen::return_(y.get() + codegen::constant<int32_t>(1));
  });

  auto module = std::move(builder).build();

  auto if_cond_ptr = module.get_address(if_cond);
  EXPECT_EQ(if_cond_ptr(8), 17);
  EXPECT_EQ(if_cond_ptr(2), 5);
}

TEST(statements, if_condition_nested) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "if_cond_nested");

  auto if_cond_nested = builder.create_function<int32_t(int32_t, int32_t)>(
      "if_cond_nested_fn", [](codegen::value<int32_t> x, codegen::value<int32_t> y) {
        auto z = codegen::variable<int32_t>{"ret"};
        codegen::if_(
            x > codegen::constant<int32_t>(4),
            [&] {
              codegen::if_(
                  y < x, [&] { z.set(x + y); }, [&] { z.set(x * y); });
            },
            [&] {
              codegen::if_(
                  y > codegen::constant<int32_t>(0), [&] { z.set(x * x); }, [&] { z.set(x - y); });
            });
        codegen::return_(z.get() + codegen::constant<int32_t>(1));
      });

  auto module = std::move(builder).build();

  auto if_cond_nested_ptr = module.get_address(if_cond_nested);
  EXPECT_EQ(if_cond_nested_ptr(8, 2), 11);
  EXPECT_EQ(if_cond_nested_ptr(8, 12), 97);
  EXPECT_EQ(if_cond_nested_ptr(2, 7), 5);
  EXPECT_EQ(if_cond_nested_ptr(2, -7), 10);
}
