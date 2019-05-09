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

namespace codegen {

void break_() {
  auto& mb = *detail::current_builder;
  assert(mb.current_loop_.break_block_);

  mb.exited_block_ = true;

  auto line_no = mb.source_code_.add_line("break;");
  mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));

  mb.ir_builder_.CreateBr(mb.current_loop_.break_block_);
}

void continue_() {
  auto& mb = *detail::current_builder;
  assert(mb.current_loop_.continue_block_);

  mb.exited_block_ = true;

  auto line_no = mb.source_code_.add_line("continue;");
  mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));

  mb.ir_builder_.CreateBr(mb.current_loop_.continue_block_);
}

} // namespace codegen
