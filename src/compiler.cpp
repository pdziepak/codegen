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

#include "os.hpp"

#include <random>

#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>

#include <llvm/ExecutionEngine/SectionMemoryManager.h>

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>

#include <llvm/Support/TargetSelect.h>

#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "codegen/module.hpp"

namespace codegen {

compiler::compiler(llvm::orc::JITTargetMachineBuilder tmb)
    : data_layout_(unwrap(tmb.getDefaultDataLayoutForTarget())), target_machine_(unwrap(tmb.createTargetMachine())),
      mangle_(session_, data_layout_), object_layer_(
                                           session_, [] { return std::make_unique<llvm::SectionMemoryManager>(); },
                                           [this](llvm::orc::VModuleKey vk, llvm::object::ObjectFile const& object,
                                                  llvm::RuntimeDyld::LoadedObjectInfo const& info) {
                                             if (gdb_listener_) { gdb_listener_->notifyObjectLoaded(vk, object, info); }
                                             loaded_modules_.emplace_back(vk);
                                           }),
      compile_layer_(session_, object_layer_, llvm::orc::ConcurrentIRCompiler(std::move(tmb))),
      optimize_layer_(session_, compile_layer_,
                      [this](llvm::orc::ThreadSafeModule tsm, llvm::orc::MaterializationResponsibility const& mr) {
                        return optimize_module(std::move(tsm), mr);
                      }),
      gdb_listener_(llvm::JITEventListener::createGDBRegistrationListener()), source_directory_([&] {
        auto eng = std::default_random_engine{std::random_device{}()};
        auto dist = std::uniform_int_distribution<uint64_t>{};
        return std::filesystem::temp_directory_path() / (get_process_name() + "-" + std::to_string(dist(eng)));
      }()),
      dynlib_generator_(unwrap(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(data_layout_))) {
  session_.getMainJITDylib().setGenerator([this](llvm::orc::JITDylib& jd,
                                                 llvm::orc::SymbolNameSet const& Names) -> llvm::orc::SymbolNameSet {
    auto added = llvm::orc::SymbolNameSet{};
    auto remaining = llvm::orc::SymbolNameSet{};
    auto new_symbols = llvm::orc::SymbolMap{};

    for (auto& name : Names) {
      auto it = external_symbols_.find(std::string(*name));
      if (it == external_symbols_.end()) {
        remaining.insert(name);
        continue;
      }
      added.insert(name);
      new_symbols[name] = llvm::JITEvaluatedSymbol(llvm::JITTargetAddress{it->second}, llvm::JITSymbolFlags::Exported);
    }
    throw_on_error(jd.define(llvm::orc::absoluteSymbols(std::move(new_symbols))));
    if (!remaining.empty()) {
      auto dynlib_added = dynlib_generator_(jd, remaining);
      added.insert(dynlib_added.begin(), dynlib_added.end());
    }
    return added;
  });

  std::filesystem::create_directories(source_directory_);
} // namespace codegen

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

compiler::~compiler() {
  for (auto vk : loaded_modules_) { gdb_listener_->notifyFreeingObject(vk); }
  std::filesystem::remove_all(source_directory_);
}

llvm::Expected<llvm::orc::ThreadSafeModule> compiler::optimize_module(llvm::orc::ThreadSafeModule tsm,
                                                                      llvm::orc::MaterializationResponsibility const&) {
  auto module = tsm.getModule();

  auto target_triple = target_machine_->getTargetTriple();

  auto library_info = std::make_unique<llvm::TargetLibraryInfoImpl>(target_triple);

  auto function_passes = llvm::legacy::FunctionPassManager(module);
  auto module_passes = llvm::legacy::PassManager();

  auto builder = llvm::PassManagerBuilder{};
  builder.OptLevel = 3;
  builder.SizeLevel = 0;
  builder.Inliner = llvm::createFunctionInliningPass();
  builder.MergeFunctions = true;
  builder.LoopVectorize = true;
  builder.SLPVectorize = true;
  builder.DisableUnrollLoops = false;
  builder.RerollLoops = true;
  builder.LibraryInfo = new llvm::TargetLibraryInfoImpl(target_triple);

  function_passes.add(llvm::createTargetTransformInfoWrapperPass(target_machine_->getTargetIRAnalysis()));
  module_passes.add(llvm::createTargetTransformInfoWrapperPass(target_machine_->getTargetIRAnalysis()));

  target_machine_->adjustPassManager(builder);

  builder.populateFunctionPassManager(function_passes);
  builder.populateModulePassManager(module_passes);

  function_passes.doInitialization();
  for (auto& func : *module) { function_passes.run(func); }
  function_passes.doFinalization();

  module_passes.run(*module);

  return tsm;
}

void compiler::add_symbol(std::string const& name, void* address) {
  external_symbols_[*mangle_(name)] = reinterpret_cast<uintptr_t>(address);
}

module::module(llvm::orc::ExecutionSession& session, llvm::DataLayout const& dl)
    : session_(&session), mangle_(session, dl) {
}

void* module::get_address(std::string const& name) {
  auto address = unwrap(session_->lookup({&session_->getMainJITDylib()}, mangle_(name))).getAddress();
  return reinterpret_cast<void*>(address);
}

} // namespace codegen
