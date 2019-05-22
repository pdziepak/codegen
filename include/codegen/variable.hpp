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

#pragma once

#include "codegen/module_builder.hpp"

namespace codegen {

template<typename Type> class variable {
  llvm::AllocaInst* variable_;
  std::string name_;

public:
  static_assert(!std::is_const_v<Type>);
  static_assert(!std::is_volatile_v<Type>);

  explicit variable(std::string const& n) : name_(n) {
    auto& mb = *detail::current_builder;

    auto alloca_builder = llvm::IRBuilder<>(&mb.function_->getEntryBlock(), mb.function_->getEntryBlock().begin());
    variable_ = alloca_builder.CreateAlloca(detail::type<Type>::llvm(), nullptr, name_);

    auto line_no = mb.source_code_.add_line(fmt::format("{} {};", detail::type<Type>::name(), name_));
    auto dbg_variable =
        mb.dbg_builder_.createAutoVariable(mb.dbg_scope_, name_, mb.dbg_file_, line_no, detail::type<Type>::dbg());
    mb.dbg_builder_.insertDeclare(variable_, dbg_variable, mb.dbg_builder_.createExpression(),
                                  llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_), mb.ir_builder_.GetInsertBlock());
  }

  template<typename Value> explicit variable(std::string const& n, Value const& v) : variable(n) { set<Value>(v); }

  variable(variable const&) = delete;
  variable(variable&&) = delete;

  value<Type> get() const {
    auto v = detail::current_builder->ir_builder_.CreateAlignedLoad(variable_, detail::type<Type>::alignment);
    return value<Type>{v, name_};
  }

  template<typename Value> void set(Value const& v) {
    static_assert(std::is_same_v<Type, typename Value::value_type>);
    auto& mb = *detail::current_builder;
    auto line_no = mb.source_code_.add_line(fmt::format("{} = {};", name_, v));
    mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
    mb.ir_builder_.CreateAlignedStore(v.eval(), variable_, detail::type<Type>::alignment);
  }
};

} // namespace codegen
