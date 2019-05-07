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

#include <llvm/Support/raw_os_ostream.h>

#include "codegen/compiler.hpp"
#include "codegen/module.hpp"

namespace codegen {

module_builder::module_builder(compiler& c, std::string const& name)
    : compiler_(&c), context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>(name, *context_)), ir_builder_(*context_), dbg_builder_(*module_) {
}

module module_builder::build() && {
  throw_on_error(compiler_->optimize_layer_.add(compiler_->session_.getMainJITDylib(),
                                                llvm::orc::ThreadSafeModule(std::move(module_), std::move(context_))));
  return module{compiler_->session_, compiler_->data_layout_};
}

std::ostream& operator<<(std::ostream& os, module_builder const& mb) {
  auto llvm_os = llvm::raw_os_ostream(os);
  mb.module_->print(llvm_os, nullptr);
  return os;
}

void return_() {
  detail::current_builder->ir_builder_.CreateRetVoid();
}

} // namespace codegen
