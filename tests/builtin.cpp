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

#include "codegen/builtin.hpp"

#include <gtest/gtest.h>

#include "codegen/compiler.hpp"
#include "codegen/module.hpp"
#include "codegen/module_builder.hpp"
#include "codegen/statements.hpp"

TEST(builtin, memcpy) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "memcpy");

  auto memcpy_i32 = builder.create_function<void(int32_t*, int32_t*)>(
      "memcpy_i32", [](codegen::value<int32_t*> src, codegen::value<int32_t*> dst) {
        codegen::builtin::memcpy(dst, src, codegen::constant<int32_t>(4));
        codegen::return_();
      });

  auto memcpy_any = builder.create_function<void(int32_t*, int32_t*, int32_t)>(
      "memcpy_any", [](codegen::value<int32_t*> src, codegen::value<int32_t*> dst, codegen::value<int32_t>(n)) {
        codegen::builtin::memcpy(dst, src, n);
        codegen::return_();
      });

  int32_t i32_src = 0x1234abcd;
  int32_t i32_dst = 0x77776666;

  auto module = std::move(builder).build();

  auto memcpy_i32_ptr = module.get_address(memcpy_i32);
  memcpy_i32_ptr(&i32_src, &i32_dst);
  EXPECT_EQ(i32_src, i32_dst);
  EXPECT_EQ(i32_dst, 0x1234abcd);

  i32_src = 0x34569876;
  auto memcpy_any_ptr = module.get_address(memcpy_any);
  memcpy_any_ptr(&i32_src, &i32_dst, 4);
  EXPECT_EQ(i32_src, i32_dst);
  EXPECT_EQ(i32_dst, 0x34569876);
}

TEST(builtin, bswap) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "bswap");

  auto bswap_i32 = builder.create_function<int32_t(int32_t)>(
      "bswap_i32", [](codegen::value<int32_t> v) { codegen::return_(codegen::builtin::bswap(v)); });
  auto module = std::move(builder).build();

  auto bswap_i32_ptr = module.get_address(bswap_i32);
  EXPECT_EQ(bswap_i32_ptr(0x12345678), 0x78563412);
}
