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

namespace detail {

enum class relational_operation_type {
  eq,
  ne,
  ge,
  gt,
  le,
  lt,
};

template<relational_operation_type Op, typename LHS, typename RHS> class relational_operation {
  LHS lhs_;
  RHS rhs_;

  using operand_type = typename LHS::value_type;
  static_assert(std::is_same_v<typename LHS::value_type, typename RHS::value_type>);

public:
  using value_type = bool;

  relational_operation(LHS lhs, RHS rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  llvm::Value* eval() const {
    if constexpr (std::is_integral_v<operand_type>) {
      switch (Op) {
      case relational_operation_type::eq: return current_builder->ir_builder_.CreateICmpEQ(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ne: return current_builder->ir_builder_.CreateICmpNE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ge:
        if constexpr (std::is_signed_v<operand_type>) {
          return current_builder->ir_builder_.CreateICmpSGE(lhs_.eval(), rhs_.eval());
        } else {
          return current_builder->ir_builder_.CreateICmpUGE(lhs_.eval(), rhs_.eval());
        }
      case relational_operation_type::gt:
        if constexpr (std::is_signed_v<operand_type>) {
          return current_builder->ir_builder_.CreateICmpSGT(lhs_.eval(), rhs_.eval());
        } else {
          return current_builder->ir_builder_.CreateICmpUGT(lhs_.eval(), rhs_.eval());
        }
      case relational_operation_type::le:
        if constexpr (std::is_signed_v<operand_type>) {
          return current_builder->ir_builder_.CreateICmpSLE(lhs_.eval(), rhs_.eval());
        } else {
          return current_builder->ir_builder_.CreateICmpULE(lhs_.eval(), rhs_.eval());
        }
      case relational_operation_type::lt:
        if constexpr (std::is_signed_v<operand_type>) {
          return current_builder->ir_builder_.CreateICmpSLT(lhs_.eval(), rhs_.eval());
        } else {
          return current_builder->ir_builder_.CreateICmpULT(lhs_.eval(), rhs_.eval());
        }
      }
    } else {
      switch (Op) {
      case relational_operation_type::eq: return current_builder->ir_builder_.CreateFCmpOEQ(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ne: return current_builder->ir_builder_.CreateFCmpONE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ge: return current_builder->ir_builder_.CreateFCmpOGE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::gt: return current_builder->ir_builder_.CreateFCmpOGT(lhs_.eval(), rhs_.eval());
      case relational_operation_type::le: return current_builder->ir_builder_.CreateFCmpOLE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::lt: return current_builder->ir_builder_.CreateFCmpOLT(lhs_.eval(), rhs_.eval());
      }
    }
  }

  friend std::ostream& operator<<(std::ostream& os, relational_operation const& ro) {
    auto symbol = [] {
      switch (Op) {
      case relational_operation_type::eq: return "==";
      case relational_operation_type::ne: return "!=";
      case relational_operation_type::ge: return ">=";
      case relational_operation_type::gt: return ">";
      case relational_operation_type::le: return "<=";
      case relational_operation_type::lt: return "<";
      }
    }();
    return os << '(' << ro.lhs_ << ' ' << symbol << ' ' << ro.rhs_ << ')';
  }
};

} // namespace detail

template<typename LHS, typename RHS,
         typename = std::enable_if_t<std::is_arithmetic_v<typename RHS::value_type> &&
                                     std::is_same_v<typename RHS::value_type, typename LHS::value_type>>>
auto operator==(LHS lhs, RHS rhs) {
  return detail::relational_operation<detail::relational_operation_type::eq, LHS, RHS>(std::move(lhs), std::move(rhs));
}

template<typename LHS, typename RHS,
         typename = std::enable_if_t<std::is_arithmetic_v<typename RHS::value_type> &&
                                     std::is_same_v<typename RHS::value_type, typename LHS::value_type>>>
auto operator!=(LHS lhs, RHS rhs) {
  return detail::relational_operation<detail::relational_operation_type::ne, LHS, RHS>(std::move(lhs), std::move(rhs));
}

template<typename LHS, typename RHS,
         typename = std::enable_if_t<std::is_arithmetic_v<typename RHS::value_type> &&
                                     std::is_same_v<typename RHS::value_type, typename LHS::value_type>>>
auto operator>=(LHS lhs, RHS rhs) {
  return detail::relational_operation<detail::relational_operation_type::ge, LHS, RHS>(std::move(lhs), std::move(rhs));
}

template<typename LHS, typename RHS,
         typename = std::enable_if_t<std::is_arithmetic_v<typename RHS::value_type> &&
                                     std::is_same_v<typename RHS::value_type, typename LHS::value_type>>>
auto operator>(LHS lhs, RHS rhs) {
  return detail::relational_operation<detail::relational_operation_type::gt, LHS, RHS>(std::move(lhs), std::move(rhs));
}

template<typename LHS, typename RHS,
         typename = std::enable_if_t<std::is_arithmetic_v<typename RHS::value_type> &&
                                     std::is_same_v<typename RHS::value_type, typename LHS::value_type>>>
auto operator<=(LHS lhs, RHS rhs) {
  return detail::relational_operation<detail::relational_operation_type::le, LHS, RHS>(std::move(lhs), std::move(rhs));
}

template<typename LHS, typename RHS,
         typename = std::enable_if_t<std::is_arithmetic_v<typename RHS::value_type> &&
                                     std::is_same_v<typename RHS::value_type, typename LHS::value_type>>>
auto operator<(LHS lhs, RHS rhs) {
  return detail::relational_operation<detail::relational_operation_type::lt, LHS, RHS>(std::move(lhs), std::move(rhs));
}

} // namespace codegen
