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

#include "codegen/module_builder.hpp"

#include <fstream>

#include <llvm/Support/raw_os_ostream.h>

#include "codegen/compiler.hpp"
#include "codegen/module.hpp"
#include <iostream>
namespace codegen {

module_builder::module_builder(compiler& c, std::string const& name)
    : compiler_(&c), context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>(name, *context_)), ir_builder_(*context_),
      source_file_(c.source_directory_ / (name + ".txt")), dbg_builder_(*module_),
      dbg_file_(dbg_builder_.createFile(source_file_.string(), source_file_.parent_path().string())),
      dbg_scope_(dbg_file_) {
  dbg_builder_.createCompileUnit(llvm::dwarf::DW_LANG_C_plus_plus, dbg_file_, "codegen", true, "", 0);
}

module module_builder::build() && {
  {
    auto ofs = std::ofstream(source_file_, std::ios::trunc);
    ofs << source_code_.get();
  }

  dbg_builder_.finalize();

  throw_on_error(compiler_->optimize_layer_.add(compiler_->session_.getMainJITDylib(),
                                                llvm::orc::ThreadSafeModule(std::move(module_), std::move(context_))));
  return module{compiler_->session_, compiler_->data_layout_};
}

unsigned module_builder::source_code_generator::add_line(std::string const& line) {
  source_code_ << std::string(indent_, ' ') << line << "\n";
  return line_no_++;
}

std::string module_builder::source_code_generator::get() const {
  return source_code_.str();
}

std::ostream& operator<<(std::ostream& os, module_builder const& mb) {
  auto llvm_os = llvm::raw_os_ostream(os);
  mb.module_->print(llvm_os, nullptr);
  return os;
}

void return_() {
  auto& mb = *detail::current_builder;
  auto line_no = mb.source_code_.add_line("return;");
  mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
  mb.ir_builder_.CreateRetVoid();
}

} // namespace codegen
