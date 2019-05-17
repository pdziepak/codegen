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

#include <random>

#include <gtest/gtest.h>

#include "codegen/arithmetic_ops.hpp"
#include "codegen/builtin.hpp"
#include "codegen/compiler.hpp"
#include "codegen/literals.hpp"
#include "codegen/module.hpp"
#include "codegen/module_builder.hpp"
#include "codegen/relational_ops.hpp"
#include "codegen/statements.hpp"
#include "codegen/variable.hpp"

namespace cg = codegen;
using namespace cg::literals;

size_t less_i32(cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr, size_t off) {
  auto a_val = cg::load(cg::bit_cast<int32_t*>(a_ptr + cg::constant<uint64_t>(off)));
  auto b_val = cg::load(cg::bit_cast<int32_t*>(b_ptr + cg::constant<uint64_t>(off)));
  cg::if_(a_val < b_val, [&] { cg::return_(cg::true_()); });
  cg::if_(a_val > b_val, [&] { cg::return_(cg::false_()); });
  return sizeof(int32_t) + off;
}
size_t less_u16(cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr, size_t off) {
  auto a_val = cg::load(cg::bit_cast<uint16_t*>(a_ptr + cg::constant<uint64_t>(off)));
  auto b_val = cg::load(cg::bit_cast<uint16_t*>(b_ptr + cg::constant<uint64_t>(off)));
  cg::if_(a_val < b_val, [&] { cg::return_(cg::true_()); });
  cg::if_(a_val > b_val, [&] { cg::return_(cg::false_()); });
  return sizeof(uint16_t) + off;
}
size_t less_f32(cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr, size_t off) {
  auto a_val = cg::load(cg::bit_cast<float*>(a_ptr + cg::constant<uint64_t>(off)));
  auto b_val = cg::load(cg::bit_cast<float*>(b_ptr + cg::constant<uint64_t>(off)));
  cg::if_(a_val < b_val, [&] { cg::return_(cg::true_()); });
  cg::if_(a_val > b_val, [&] { cg::return_(cg::false_()); });
  return sizeof(float) + off;
}

TEST(examples, tuple_i32f32u16_less) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "tuple_i32f32u16_less");
  auto less = builder.create_function<bool(std::byte*, std::byte*)>(
      "less", [&](cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr) {
        size_t offset = 0;
        offset = less_i32(a_ptr, b_ptr, offset);
        offset = less_f32(a_ptr, b_ptr, offset);
        offset = less_u16(a_ptr, b_ptr, offset);
        (void)offset;
        cg::return_(cg::false_());
      });
  auto module = std::move(builder).build();
  auto less_ptr = module.get_address(less);

  auto make_tuple = [](int32_t a, float b, uint16_t c) {
    auto data = std::make_unique<std::byte[]>(sizeof(a) + sizeof(b) + sizeof(c));
    auto dst = data.get();
    dst = std::copy_n(reinterpret_cast<std::byte const*>(&a), sizeof(a), dst);
    dst = std::copy_n(reinterpret_cast<std::byte const*>(&b), sizeof(b), dst);
    dst = std::copy_n(reinterpret_cast<std::byte const*>(&c), sizeof(c), dst);
    (void)dst;
    return data;
  };

  EXPECT_TRUE(less_ptr(make_tuple(0, 2.5f, 1).get(), make_tuple(1, 2.5f, 2).get()));
  EXPECT_TRUE(less_ptr(make_tuple(1, 2, 1).get(), make_tuple(1, 2.5f, 2).get()));
  EXPECT_TRUE(less_ptr(make_tuple(1, 2.5f, 1).get(), make_tuple(1, 2.5f, 2).get()));
  EXPECT_FALSE(less_ptr(make_tuple(1, 2.5f, 2).get(), make_tuple(1, 2.5f, 2).get()));
  EXPECT_FALSE(less_ptr(make_tuple(1, 2.5f, 2).get(), make_tuple(-1, 2.5f, 2).get()));
  EXPECT_FALSE(less_ptr(make_tuple(1, 2.5f, 2).get(), make_tuple(1, -2.5f, 2).get()));
  EXPECT_FALSE(less_ptr(make_tuple(1, 2.5f, 2).get(), make_tuple(1, 2.5f, 0).get()));
}

TEST(examples, tuple_i32str_less) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "tuple_i32str_less");

  auto min =
      builder.create_function<uint32_t(uint32_t, uint32_t)>("min", [&](cg::value<uint32_t> a, cg::value<uint32_t> b) {
        cg::if_(a < b, [&] { cg::return_(a); });
        cg::return_(b);
      });

  auto less = builder.create_function<bool(std::byte*, std::byte*)>(
      "less", [&](cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr) {
        size_t offset = 0;
        offset = less_i32(a_ptr, b_ptr, offset);

        auto a_len = cg::load(cg::bit_cast<uint32_t*>(a_ptr + cg::constant<uint64_t>(offset)));
        auto b_len = cg::load(cg::bit_cast<uint32_t*>(b_ptr + cg::constant<uint64_t>(offset)));
        auto len = cg::call(min, a_len, b_len);
        auto ret = cg::builtin::memcmp(a_ptr + cg::constant<uint64_t>(offset) + 4_u64,
                                       b_ptr + cg::constant<uint64_t>(offset) + 4_u64, len);
        cg::if_(ret < 0_i32, [&] { cg::return_(cg::true_()); });
        cg::if_(ret > 0_i32, [&] { cg::return_(cg::false_()); });
        cg::return_(a_len < b_len);
      });

  auto module = std::move(builder).build();
  auto less_ptr = module.get_address(less);

  auto make_tuple = [](int32_t a, std::string_view b) {
    auto data = std::make_unique<std::byte[]>(sizeof(a) + sizeof(uint32_t) + b.size());
    uint32_t b_len = b.size();
    auto dst = data.get();
    dst = std::copy_n(reinterpret_cast<std::byte const*>(&a), sizeof(a), dst);
    dst = std::copy_n(reinterpret_cast<std::byte const*>(&b_len), sizeof(b_len), dst);
    dst = std::copy_n(reinterpret_cast<std::byte const*>(b.data()), b.size(), dst);
    (void)dst;
    return data;
  };

  EXPECT_TRUE(less_ptr(make_tuple(0, "bbb").get(), make_tuple(1, "bbb").get()));
  EXPECT_TRUE(less_ptr(make_tuple(1, "aaa").get(), make_tuple(1, "bbb").get()));
  EXPECT_TRUE(less_ptr(make_tuple(1, "aa").get(), make_tuple(1, "aaa").get()));
  EXPECT_TRUE(less_ptr(make_tuple(1, "aaa").get(), make_tuple(1, "z").get()));
  EXPECT_FALSE(less_ptr(make_tuple(1, "bbb").get(), make_tuple(1, "bbb").get()));
  EXPECT_FALSE(less_ptr(make_tuple(1, "bbb").get(), make_tuple(-1, "bbb").get()));
  EXPECT_FALSE(less_ptr(make_tuple(1, "bbb").get(), make_tuple(1, "abc").get()));
  EXPECT_FALSE(less_ptr(make_tuple(1, "bbb").get(), make_tuple(1, "bb").get()));
  EXPECT_FALSE(less_ptr(make_tuple(1, "z").get(), make_tuple(1, "bbb").get()));
}

TEST(examples, soa_compute) {
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "soa_compute");
  auto compute = builder.create_function<void(int32_t, int32_t*, int32_t*, int32_t*, uint64_t)>(
      "compute", [&](cg::value<int32_t> a, cg::value<int32_t*> b_ptr, cg::value<int32_t*> c_ptr,
                     cg::value<int32_t*> d_ptr, cg::value<uint64_t> n) {
        auto idx = cg::variable<uint64_t>("idx", 0_u64);
        cg::while_([&] { return idx.get() < n; },
                   [&] {
                     auto i = idx.get();
                     cg::store(a * cg::load(b_ptr + i) + cg::load(c_ptr + i), d_ptr + i);
                     idx.set(i + 1_u64);
                   });
        cg::return_();
      });

  auto module = std::move(builder).build();
  auto compute_ptr = module.get_address(compute);
  compute_ptr(0, nullptr, nullptr, nullptr, 0);

  auto test = [&](int32_t a, std::vector<int32_t> b, std::vector<int32_t> c) {
    EXPECT_EQ(b.size(), c.size());
    auto d = std::make_unique<int32_t[]>(b.size());
    compute_ptr(a, b.data(), c.data(), d.get(), b.size());
    for (auto i = 0u; i < b.size(); i++) { EXPECT_EQ(d[i], a * b[i] + c[i]); }
  };

  test(2, {1, 2, 3, 4, 5, 6}, {11, 12, 13, 14, 15, 16});
  test(5, {-8, 5, -4, 3, -10, 11}, {0, 8, 3, -9, 4, 2});

  auto gen = std::default_random_engine{std::random_device{}()};
  auto dist = std::uniform_int_distribution<int32_t>(-10000, 10000);

  auto b = std::vector<int32_t>();
  auto c = std::vector<int32_t>();
  std::generate_n(std::back_inserter(b), 1000000, [&] { return dist(gen); });
  std::generate_n(std::back_inserter(c), 1000000, [&] { return dist(gen); });
  test(dist(gen), std::move(b), std::move(c));
}
