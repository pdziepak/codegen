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

#include "codegen/relational_ops.hpp"

#include <gtest/gtest.h>

#include "codegen/compiler.hpp"
#include "codegen/module.hpp"
#include "codegen/module_builder.hpp"

TEST(relational_ops, signed_integer) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "signed_integer");

  auto eq2 = builder.create_function<bool(int32_t, int32_t)>(
      "eq2", [](codegen::value<int32_t> x, codegen::value<int32_t> y) { codegen::return_(x == y); });

  auto ne2 = builder.create_function<bool(int32_t, int32_t)>(
      "ne2", [](codegen::value<int32_t> x, codegen::value<int32_t> y) { codegen::return_(x != y); });

  auto ge2 = builder.create_function<bool(int32_t, int32_t)>(
      "ge2", [](codegen::value<int32_t> x, codegen::value<int32_t> y) { codegen::return_(x >= y); });

  auto gt2 = builder.create_function<bool(int32_t, int32_t)>(
      "gt2", [](codegen::value<int32_t> x, codegen::value<int32_t> y) { codegen::return_(x > y); });

  auto le2 = builder.create_function<bool(int32_t, int32_t)>(
      "le2", [](codegen::value<int32_t> x, codegen::value<int32_t> y) { codegen::return_(x <= y); });

  auto lt2 = builder.create_function<bool(int32_t, int32_t)>(
      "lt2", [](codegen::value<int32_t> x, codegen::value<int32_t> y) { codegen::return_(x < y); });

  auto module = std::move(builder).build();

  auto eq2_ptr = module.get_address(eq2);
  EXPECT_EQ(eq2_ptr(2, 2), true);
  EXPECT_EQ(eq2_ptr(1, 3), false);

  auto ne2_ptr = module.get_address(ne2);
  EXPECT_EQ(ne2_ptr(2, 2), false);
  EXPECT_EQ(ne2_ptr(1, 3), true);

  auto ge2_ptr = module.get_address(ge2);
  EXPECT_EQ(ge2_ptr(2, 2), true);
  EXPECT_EQ(ge2_ptr(1, 3), false);
  EXPECT_EQ(ge2_ptr(5, 4), true);
  EXPECT_EQ(ge2_ptr(-1, 1), false);
  EXPECT_EQ(ge2_ptr(-1, -3), true);
  EXPECT_EQ(ge2_ptr(-5, -4), false);

  auto gt2_ptr = module.get_address(gt2);
  EXPECT_EQ(gt2_ptr(2, 2), false);
  EXPECT_EQ(gt2_ptr(1, 3), false);
  EXPECT_EQ(gt2_ptr(5, 4), true);
  EXPECT_EQ(gt2_ptr(-1, 1), false);
  EXPECT_EQ(gt2_ptr(-1, -3), true);
  EXPECT_EQ(gt2_ptr(-5, -4), false);

  auto le2_ptr = module.get_address(le2);
  EXPECT_EQ(le2_ptr(2, 2), true);
  EXPECT_EQ(le2_ptr(1, 3), true);
  EXPECT_EQ(le2_ptr(5, 4), false);
  EXPECT_EQ(le2_ptr(-1, 1), true);
  EXPECT_EQ(le2_ptr(-1, -3), false);
  EXPECT_EQ(le2_ptr(-5, -4), true);

  auto lt2_ptr = module.get_address(lt2);
  EXPECT_EQ(lt2_ptr(2, 2), false);
  EXPECT_EQ(lt2_ptr(1, 3), true);
  EXPECT_EQ(lt2_ptr(5, 4), false);
  EXPECT_EQ(lt2_ptr(-1, 1), true);
  EXPECT_EQ(lt2_ptr(-1, -3), false);
  EXPECT_EQ(lt2_ptr(-5, -4), true);
}

TEST(relational_ops, unsigned_integer) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "unsigned_integer");

  auto eq2 = builder.create_function<bool(uint32_t, uint32_t)>(
      "eq2", [](codegen::value<uint32_t> x, codegen::value<uint32_t> y) { codegen::return_(x == y); });

  auto ne2 = builder.create_function<bool(uint32_t, uint32_t)>(
      "ne2", [](codegen::value<uint32_t> x, codegen::value<uint32_t> y) { codegen::return_(x != y); });

  auto ge2 = builder.create_function<bool(uint32_t, uint32_t)>(
      "ge2", [](codegen::value<uint32_t> x, codegen::value<uint32_t> y) { codegen::return_(x >= y); });

  auto gt2 = builder.create_function<bool(uint32_t, uint32_t)>(
      "gt2", [](codegen::value<uint32_t> x, codegen::value<uint32_t> y) { codegen::return_(x > y); });

  auto le2 = builder.create_function<bool(uint32_t, uint32_t)>(
      "le2", [](codegen::value<uint32_t> x, codegen::value<uint32_t> y) { codegen::return_(x <= y); });

  auto lt2 = builder.create_function<bool(uint32_t, uint32_t)>(
      "lt2", [](codegen::value<uint32_t> x, codegen::value<uint32_t> y) { codegen::return_(x < y); });

  auto module = std::move(builder).build();

  auto eq2_ptr = module.get_address(eq2);
  EXPECT_EQ(eq2_ptr(2, 2), true);
  EXPECT_EQ(eq2_ptr(1, 3), false);

  auto ne2_ptr = module.get_address(ne2);
  EXPECT_EQ(ne2_ptr(2, 2), false);
  EXPECT_EQ(ne2_ptr(1, 3), true);

  auto ge2_ptr = module.get_address(ge2);
  EXPECT_EQ(ge2_ptr(2, 2), true);
  EXPECT_EQ(ge2_ptr(1, 3), false);
  EXPECT_EQ(ge2_ptr(5, 4), true);
  EXPECT_EQ(ge2_ptr(-1, 1), true);
  EXPECT_EQ(ge2_ptr(-1, -3), true);
  EXPECT_EQ(ge2_ptr(-5, -4), false);

  auto gt2_ptr = module.get_address(gt2);
  EXPECT_EQ(gt2_ptr(2, 2), false);
  EXPECT_EQ(gt2_ptr(1, 3), false);
  EXPECT_EQ(gt2_ptr(5, 4), true);
  EXPECT_EQ(gt2_ptr(-1, 1), true);
  EXPECT_EQ(gt2_ptr(-1, -3), true);
  EXPECT_EQ(gt2_ptr(-5, -4), false);

  auto le2_ptr = module.get_address(le2);
  EXPECT_EQ(le2_ptr(2, 2), true);
  EXPECT_EQ(le2_ptr(1, 3), true);
  EXPECT_EQ(le2_ptr(5, 4), false);
  EXPECT_EQ(le2_ptr(-1, 1), false);
  EXPECT_EQ(le2_ptr(-1, -3), false);
  EXPECT_EQ(le2_ptr(-5, -4), true);

  auto lt2_ptr = module.get_address(lt2);
  EXPECT_EQ(lt2_ptr(2, 2), false);
  EXPECT_EQ(lt2_ptr(1, 3), true);
  EXPECT_EQ(lt2_ptr(5, 4), false);
  EXPECT_EQ(lt2_ptr(-1, 1), false);
  EXPECT_EQ(lt2_ptr(-1, -3), false);
  EXPECT_EQ(lt2_ptr(-5, -4), true);
}
