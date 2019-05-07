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

#include "codegen/compiler.hpp"

#include <llvm/ExecutionEngine/SectionMemoryManager.h>

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>

#include <llvm/Support/TargetSelect.h>

#include "codegen/module.hpp"

namespace codegen {

compiler::compiler(llvm::orc::JITTargetMachineBuilder tmb)
    : data_layout_(unwrap(tmb.getDefaultDataLayoutForTarget())),
      object_layer_(
          session_, [] { return std::make_unique<llvm::SectionMemoryManager>(); },
          [this](llvm::orc::VModuleKey vk, llvm::object::ObjectFile const& object,
                 llvm::RuntimeDyld::LoadedObjectInfo const& info) {
            if (gdb_listener_) { gdb_listener_->notifyObjectLoaded(vk, object, info); }
          }),
      compile_layer_(session_, object_layer_, llvm::orc::ConcurrentIRCompiler(std::move(tmb))),
      optimize_layer_(session_, compile_layer_, optimize_module),
      gdb_listener_(llvm::JITEventListener::createGDBRegistrationListener()) {
  auto gen = llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(data_layout_);
  session_.getMainJITDylib().setGenerator(*gen);
}

compiler::compiler()
    : compiler([] {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto tmb = unwrap(llvm::orc::JITTargetMachineBuilder::detectHost());
        tmb.setCodeGenOptLevel(llvm::CodeGenOpt::Aggressive);
        tmb.setCPU(llvm::sys::getHostCPUName());
        return tmb;
      }()) {
}

llvm::Expected<llvm::orc::ThreadSafeModule> compiler::optimize_module(llvm::orc::ThreadSafeModule module,
                                                                      llvm::orc::MaterializationResponsibility const&) {
  return module;
}

module::module(llvm::orc::ExecutionSession& session, llvm::DataLayout dl) : session_(&session), mangle_(session, dl) {
}

void* module::get_address(std::string const& name) {
  auto address = unwrap(session_->lookup({&session_->getMainJITDylib()}, mangle_(name))).getAddress();
  return reinterpret_cast<void*>(address);
}

} // namespace codegen
