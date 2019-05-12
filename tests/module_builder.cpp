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
#include "codegen/literals.hpp"
#include "codegen/module.hpp"
#include "codegen/module_builder.hpp"
#include "codegen/statements.hpp"

using namespace codegen::literals;

TEST(module_builder, empty) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "empty");
  auto module = std::move(builder).build();
  (void)module;
}

TEST(module_builder, return_void) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "return_void");
  auto fn = builder.create_function<void()>("return_void_fn", [] { codegen::return_(); });
  auto module = std::move(builder).build();
  auto fn_ptr = module.get_address(fn);
  fn_ptr();
}

TEST(module_builder, return_i32) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "return_i32");

  auto return_constant = builder.create_function<int32_t()>("return_constant", [] { codegen::return_(4_i32); });

  auto return_argument = builder.create_function<int32_t(int32_t)>(
      "return_argument", [](codegen::value<int32_t> arg) { codegen::return_(arg); });

  auto module = std::move(builder).build();

  auto return_constant_ptr = module.get_address(return_constant);
  EXPECT_EQ(return_constant_ptr(), 4);

  auto return_argument_ptr = module.get_address(return_argument);
  EXPECT_EQ(return_argument_ptr(1), 1);
  EXPECT_EQ(return_argument_ptr(8), 8);
  EXPECT_EQ(return_argument_ptr(-7), -7);
}

TEST(module_builder, external_functions) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "external_functions");

  bool called = false;
  auto callee = builder.declare_external_function<void(bool*)>("set_true", [](bool* flag) { *flag = true; });

  auto caller = builder.create_function<void(bool*)>("caller", [&](codegen::value<bool*> f) {
    codegen::call(callee, f);
    codegen::return_();
  });

  auto module = std::move(builder).build();
  auto caller_ptr = module.get_address(caller);
  caller_ptr(&called);
  EXPECT_TRUE(called);

  called = false;
  auto callee_ptr = module.get_address(callee);
  callee_ptr(&called);
  EXPECT_TRUE(called);
}

TEST(module_builder, bit_cast) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "bit_cast");

  auto intptr_to_voidptr = builder.create_function<void*(int32_t*)>(
      "intptr_to_voidptr", [&](codegen::value<int32_t*> ptr) { codegen::return_(codegen::bit_cast<void*>(ptr)); });

  auto module = std::move(builder).build();
  auto intptr_to_voidptr_ptr = module.get_address(intptr_to_voidptr);
  int32_t value;
  EXPECT_EQ(reinterpret_cast<uintptr_t>(intptr_to_voidptr_ptr(&value)), reinterpret_cast<uintptr_t>(&value));
}

TEST(module_builder, cast) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "cast");

  auto f32_to_i16 = builder.create_function<int16_t(float)>(
      "f32_to_i16", [&](codegen::value<float> x) { codegen::return_(codegen::cast<int16_t>(x)); });

  auto i32_to_f64 = builder.create_function<double(int32_t)>(
      "i32_to_f64", [&](codegen::value<int32_t> x) { codegen::return_(codegen::cast<double>(x)); });

  auto i16_to_i64 = builder.create_function<int64_t(int16_t)>(
      "i16_to_i64", [&](codegen::value<int16_t> x) { codegen::return_(codegen::cast<int64_t>(x)); });

  auto u16_to_u64 = builder.create_function<uint64_t(uint16_t)>(
      "u16_to_u64", [&](codegen::value<uint16_t> x) { codegen::return_(codegen::cast<uint64_t>(x)); });

  auto module = std::move(builder).build();

  auto f32_to_i16_ptr = module.get_address(f32_to_i16);
  EXPECT_EQ(f32_to_i16_ptr(3.5f), 3);

  auto i32_to_f64_ptr = module.get_address(i32_to_f64);
  EXPECT_EQ(i32_to_f64_ptr(4), 4.);

  auto i16_to_i64_ptr = module.get_address(i16_to_i64);
  EXPECT_EQ(i16_to_i64_ptr(-1), -1);

  auto u16_to_u64_ptr = module.get_address(u16_to_u64);
  EXPECT_EQ(u16_to_u64_ptr(-1), 0xffff);
}
