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

#include "codegen/arithmetic_ops.hpp"

#include <gtest/gtest.h>

#include "codegen/compiler.hpp"
#include "codegen/module.hpp"
#include "codegen/module_builder.hpp"

TEST(arithmetic_ops, signed_integer_arithmetic) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "signed_integer_arithmetic");

  auto add2 = builder.create_function<int32_t(int32_t, int32_t)>(
      "add2", [](codegen::value<int32_t> x, codegen::value<int32_t> y) { codegen::return_(x + y); });

  auto add4 = builder.create_function<int32_t(int32_t, int32_t, int32_t, int32_t)>(
      "add4", [](codegen::value<int32_t> x, codegen::value<int32_t> y, codegen::value<int32_t> z,
                 codegen::value<int32_t> w) { codegen::return_(x + y + z + w); });

  auto sub_add4 = builder.create_function<int32_t(int32_t, int32_t, int32_t, int32_t)>(
      "sub_add4", [](codegen::value<int32_t> x, codegen::value<int32_t> y, codegen::value<int32_t> z,
                     codegen::value<int32_t> w) { codegen::return_(x - y + z - w); });

  auto mul_div_mod2 = builder.create_function<int32_t(int32_t, int32_t)>(
      "mul_div_mod2",
      [](codegen::value<int32_t> x, codegen::value<int32_t> y) { codegen::return_((x / y) * y + x % y); });

  auto module = std::move(builder).build();

  auto add2_ptr = module.get_address(add2);
  EXPECT_EQ(add2_ptr(1, 2), 3);

  auto add4_ptr = module.get_address(add4);
  EXPECT_EQ(add4_ptr(1, 2, 3, 4), 10);

  auto sub_add4_ptr = module.get_address(sub_add4);
  EXPECT_EQ(sub_add4_ptr(1, 2, 3, 4), -2);

  auto mul_div_mod2_ptr = module.get_address(mul_div_mod2);
  EXPECT_EQ(mul_div_mod2_ptr(7, 2), 7);
  EXPECT_EQ(mul_div_mod2_ptr(11, 3), 11);
  EXPECT_EQ(mul_div_mod2_ptr(4, -3), 4);
  EXPECT_EQ(mul_div_mod2_ptr(1, -7), 1);
}

TEST(arithmetic_ops, unsigned_integer_arithmetic) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "unsigned_integer_arithmetic");

  auto add2 = builder.create_function<uint32_t(uint32_t, uint32_t)>(
      "add2", [](codegen::value<uint32_t> x, codegen::value<uint32_t> y) { codegen::return_(x + y); });

  auto add4 = builder.create_function<uint32_t(uint32_t, uint32_t, uint32_t, uint32_t)>(
      "add4", [](codegen::value<uint32_t> x, codegen::value<uint32_t> y, codegen::value<uint32_t> z,
                 codegen::value<uint32_t> w) { codegen::return_(x + y + z + w); });

  auto sub_add4 = builder.create_function<uint32_t(uint32_t, uint32_t, uint32_t, uint32_t)>(
      "sub_add4", [](codegen::value<uint32_t> x, codegen::value<uint32_t> y, codegen::value<uint32_t> z,
                     codegen::value<uint32_t> w) { codegen::return_(x - y + z - w); });

  auto mul_div_mod2 = builder.create_function<uint32_t(uint32_t, uint32_t)>(
      "mul_div_mod2",
      [](codegen::value<uint32_t> x, codegen::value<uint32_t> y) { codegen::return_((x / y) * y + x % y); });

  auto module = std::move(builder).build();

  auto add2_ptr = module.get_address(add2);
  EXPECT_EQ(add2_ptr(1, 2), 3);

  auto add4_ptr = module.get_address(add4);
  EXPECT_EQ(add4_ptr(1, 2, 3, 4), 10);

  auto sub_add4_ptr = module.get_address(sub_add4);
  EXPECT_EQ(sub_add4_ptr(1, 2, 3, 4), uint32_t(-2));

  auto mul_div_mod2_ptr = module.get_address(mul_div_mod2);
  EXPECT_EQ(mul_div_mod2_ptr(7, 2), 7);
  EXPECT_EQ(mul_div_mod2_ptr(11, 3), 11);
  EXPECT_EQ(mul_div_mod2_ptr(4, uint32_t(-3)), 4);
  EXPECT_EQ(mul_div_mod2_ptr(1, uint32_t(-7)), 1);
}

TEST(arithmetic_ops, float_arithmetic) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "float_arithmetic");

  auto add2 = builder.create_function<float(float, float)>(
      "add2", [](codegen::value<float> x, codegen::value<float> y) { codegen::return_(x + y + codegen::constant<float>(0.5f)); });

  auto add4 = builder.create_function<float(float, float, float, float)>(
      "add4", [](codegen::value<float> x, codegen::value<float> y, codegen::value<float> z, codegen::value<float> w) {
        codegen::return_(x + y + z + w);
      });

  auto sub_add4 = builder.create_function<float(float, float, float, float)>(
      "sub_add4", [](codegen::value<float> x, codegen::value<float> y, codegen::value<float> z,
                     codegen::value<float> w) { codegen::return_(x - y + z - w); });

  auto mul_div_mod2 = builder.create_function<float(float, float)>(
      "mul_div_mod2", [](codegen::value<float> x, codegen::value<float> y) { codegen::return_((x / y) * y + x % y); });

  auto module = std::move(builder).build();

  auto add2_ptr = module.get_address(add2);
  EXPECT_EQ(add2_ptr(1, 2), 3.5f);

  auto add4_ptr = module.get_address(add4);
  EXPECT_EQ(add4_ptr(1, 2, 3, 4), 10);

  auto sub_add4_ptr = module.get_address(sub_add4);
  EXPECT_EQ(sub_add4_ptr(1, 2, 3, 4), -2);

  auto mul_div_mod2_ptr = module.get_address(mul_div_mod2);
  EXPECT_EQ(mul_div_mod2_ptr(7, 2), 8);
  EXPECT_EQ(mul_div_mod2_ptr(11, 3), 13);
  EXPECT_EQ(mul_div_mod2_ptr(4, -3), 5);
  EXPECT_EQ(mul_div_mod2_ptr(1, -7), 2);
}

TEST(arithmetic_ops, signed_integer_bitwise) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "signed_integer_bitwise");

  auto and2 = builder.create_function<int32_t(int32_t, int32_t)>(
      "and2", [](codegen::value<int32_t> x, codegen::value<int32_t> y) { codegen::return_(x & y); });

  auto and4 = builder.create_function<int32_t(int32_t, int32_t, int32_t, int32_t)>(
      "and4", [](codegen::value<int32_t> x, codegen::value<int32_t> y, codegen::value<int32_t> z,
                 codegen::value<int32_t> w) { codegen::return_(x & y & z & w); });

  auto and_or4 = builder.create_function<int32_t(int32_t, int32_t, int32_t, int32_t)>(
      "and_or4", [](codegen::value<int32_t> x, codegen::value<int32_t> y, codegen::value<int32_t> z,
                    codegen::value<int32_t> w) { codegen::return_((x & y) | (z & w)); });

  auto and_or_xor4 = builder.create_function<int32_t(int32_t, int32_t, int32_t, int32_t)>(
      "and_or_xor4", [](codegen::value<int32_t> x, codegen::value<int32_t> y, codegen::value<int32_t> z,
                        codegen::value<int32_t> w) { codegen::return_((x | y) ^ (z & w)); });

  auto module = std::move(builder).build();

  auto and2_ptr = module.get_address(and2);
  EXPECT_EQ(and2_ptr(1, 2), 0);
  EXPECT_EQ(and2_ptr(1, 3), 1);

  auto and4_ptr = module.get_address(and4);
  EXPECT_EQ(and4_ptr(3, 3, 7, 2), 2);

  auto and_or4_ptr = module.get_address(and_or4);
  EXPECT_EQ(and_or4_ptr(0x10, 0x30, 3, 6), 0x12);

  auto and_or_xor4_ptr = module.get_address(and_or_xor4);
  EXPECT_EQ(and_or_xor4_ptr(3, 6, 11, 14), 13);
}
