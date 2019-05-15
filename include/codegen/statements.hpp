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
#include "codegen/utils.hpp"

namespace codegen {

template<typename Condition, typename TrueBlock, typename FalseBlock,
         typename = std::enable_if_t<std::is_same_v<typename std::decay_t<Condition>::value_type, bool>>>
void if_(Condition&& cnd, TrueBlock&& tb, FalseBlock&& fb) {
  auto& mb = *detail::current_builder;

  auto line_no = mb.source_code_.add_line(fmt::format("if ({}) {{", cnd));
  mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));

  auto true_block = llvm::BasicBlock::Create(*mb.context_, "true_block", mb.function_);
  auto false_block = llvm::BasicBlock::Create(*mb.context_, "false_block");
  auto merge_block = llvm::BasicBlock::Create(*mb.context_, "merge_block");

  mb.ir_builder_.CreateCondBr(cnd.eval(), true_block, false_block);

  mb.ir_builder_.SetInsertPoint(true_block);
  mb.source_code_.enter_scope();

  auto scope = mb.dbg_builder_.createLexicalBlock(mb.dbg_scope_, mb.dbg_file_, mb.source_code_.current_line(), 1);
  auto parent_scope = std::exchange(mb.dbg_scope_, scope);

  assert(!mb.exited_block_);
  tb();
  mb.source_code_.leave_scope();

  line_no = mb.source_code_.add_line("} else {");

  if (!mb.exited_block_) {
    mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, parent_scope));
    mb.ir_builder_.CreateBr(merge_block);
  }
  mb.exited_block_ = false;

  mb.function_->getBasicBlockList().push_back(false_block);
  mb.ir_builder_.SetInsertPoint(false_block);
  mb.source_code_.enter_scope();

  mb.dbg_scope_ = mb.dbg_builder_.createLexicalBlock(parent_scope, mb.dbg_file_, mb.source_code_.current_line(), 1);

  fb();
  mb.source_code_.leave_scope();

  mb.dbg_scope_ = parent_scope;

  line_no = mb.source_code_.add_line("}");

  if (!mb.exited_block_) {
    mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
    mb.ir_builder_.CreateBr(merge_block);
  }
  mb.exited_block_ = false;

  mb.function_->getBasicBlockList().push_back(merge_block);
  mb.ir_builder_.SetInsertPoint(merge_block);
}

template<typename Condition, typename TrueBlock,
         typename = std::enable_if_t<std::is_same_v<typename std::decay_t<Condition>::value_type, bool>>>
void if_(Condition&& cnd, TrueBlock&& tb) {
  auto& mb = *detail::current_builder;

  auto line_no = mb.source_code_.add_line(fmt::format("if ({}) {{", cnd));
  mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));

  auto true_block = llvm::BasicBlock::Create(*mb.context_, "true_block", mb.function_);
  auto merge_block = llvm::BasicBlock::Create(*mb.context_, "merge_block");

  mb.ir_builder_.CreateCondBr(cnd.eval(), true_block, merge_block);

  mb.ir_builder_.SetInsertPoint(true_block);
  mb.source_code_.enter_scope();

  auto scope = mb.dbg_builder_.createLexicalBlock(mb.dbg_scope_, mb.dbg_file_, mb.source_code_.current_line(), 1);
  auto parent_scope = std::exchange(mb.dbg_scope_, scope);

  assert(!mb.exited_block_);
  tb();
  mb.source_code_.leave_scope();

  mb.dbg_scope_ = parent_scope;

  line_no = mb.source_code_.add_line("}");

  if (!mb.exited_block_) {
    mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
    mb.ir_builder_.CreateBr(merge_block);
  }
  mb.exited_block_ = false;

  mb.function_->getBasicBlockList().push_back(merge_block);
  mb.ir_builder_.SetInsertPoint(merge_block);
}

template<typename ReturnType, typename... Arguments, typename... Values>
value<ReturnType> call(function_ref<ReturnType, Arguments...> const& fn, Values&&... args) {
  static_assert((std::is_same_v<Arguments, typename std::decay_t<Values>::value_type> && ...));

  auto& mb = *detail::current_builder;

  auto str = std::stringstream{};
  str << fn.name() << "_ret = " << fn.name() << "(";
  (void)(str << ... << fmt::format("{}, ", args));
  str << ");";
  auto line_no = mb.source_code_.add_line(str.str());

  mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));

  auto values = std::vector<llvm::Value*>{};
  [[maybe_unused]] auto _ = {0, ((values.emplace_back(args.eval())), 0)...};

  auto ret = mb.ir_builder_.CreateCall(fn, values);
  return value<ReturnType>{ret, fmt::format("{}_ret", fn.name())};
}

template<typename Pointer, typename = std::enable_if_t<std::is_pointer_v<typename std::decay_t<Pointer>::value_type>>>
auto load(Pointer ptr) {
  using value_type = std::remove_pointer_t<typename std::decay_t<Pointer>::value_type>;
  auto& mb = *detail::current_builder;

  auto id = fmt::format("val{}", detail::id_counter++);

  auto line_no = mb.source_code_.add_line(fmt::format("{} = *{}", id, ptr));
  mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
  auto v = mb.ir_builder_.CreateAlignedLoad(ptr.eval(), detail::type<value_type>::alignment);

  auto dbg_value =
      mb.dbg_builder_.createAutoVariable(mb.dbg_scope_, id, mb.dbg_file_, line_no, detail::type<value_type>::dbg());
  mb.dbg_builder_.insertDbgValueIntrinsic(v, dbg_value, mb.dbg_builder_.createExpression(),
                                          llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_),
                                          mb.ir_builder_.GetInsertBlock());

  return value<value_type>{v, id};
}

template<typename Value, typename Pointer,
         typename = std::enable_if_t<std::is_pointer_v<typename std::decay_t<Pointer>::value_type> &&
                                     std::is_same_v<typename std::decay_t<Value>::value_type,
                                                    std::remove_pointer_t<typename std::decay_t<Pointer>::value_type>>>>
void store(Value v, Pointer ptr) {
  using value_type = std::remove_pointer_t<typename std::decay_t<Pointer>::value_type>;
  auto& mb = *detail::current_builder;

  auto line_no = mb.source_code_.add_line(fmt::format("*{} = {}", ptr, v));
  mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
  mb.ir_builder_.CreateAlignedStore(v.eval(), ptr.eval(), detail::type<value_type>::alignment);
}

template<typename ConditionFn, typename Body,
         typename = std::enable_if_t<std::is_same_v<typename std::invoke_result_t<ConditionFn>::value_type, bool>>>
void while_(ConditionFn cnd_fn, Body bdy) {
  auto& mb = *detail::current_builder;

  auto line_no = mb.source_code_.current_line() + 1;
  mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
  auto cnd = cnd_fn();
  mb.source_code_.add_line(fmt::format("while ({}) {{", cnd));

  auto while_continue = llvm::BasicBlock::Create(*mb.context_, "while_continue", mb.function_);
  auto while_iteration = llvm::BasicBlock::Create(*mb.context_, "while_iteration");
  auto while_break = llvm::BasicBlock::Create(*mb.context_, "while_break");

  auto parent_loop = std::exchange(mb.current_loop_, module_builder::loop{while_continue, while_break});

  mb.ir_builder_.CreateBr(while_continue);
  mb.ir_builder_.SetInsertPoint(while_continue);

  mb.ir_builder_.CreateCondBr(cnd_fn().eval(), while_iteration, while_break);

  mb.source_code_.enter_scope();

  auto scope = mb.dbg_builder_.createLexicalBlock(mb.dbg_scope_, mb.dbg_file_, mb.source_code_.current_line(), 1);
  auto parent_scope = std::exchange(mb.dbg_scope_, scope);

  mb.function_->getBasicBlockList().push_back(while_iteration);
  mb.ir_builder_.SetInsertPoint(while_iteration);

  assert(!mb.exited_block_);
  bdy();

  mb.dbg_scope_ = parent_scope;

  mb.source_code_.leave_scope();
  line_no = mb.source_code_.add_line("}");

  if (!mb.exited_block_) {
    mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
    mb.ir_builder_.CreateBr(while_continue);
  }
  mb.exited_block_ = false;

  mb.function_->getBasicBlockList().push_back(while_break);
  mb.ir_builder_.SetInsertPoint(while_break);

  mb.current_loop_ = parent_loop;
}

void break_();
void continue_();

} // namespace codegen
