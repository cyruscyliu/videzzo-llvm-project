//===- ViDeZZoInstrumentation.cpp - The -videzzoinstrumentation pass    ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass instruments functions required by ViDeZZo.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/InitializePasses.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/Path.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include <iostream>

using namespace llvm;

//
// callstack
// OTHER_CONTEXT, PARENT_FUNCTION, TARGET_FUNCTION, TARGET_FUNCTION_INDEX, ADDR_ARGUMENT_INDEX
//
#define POS_PARENT_FUNCTION             (-4)
#define POS_TARGET_FUNCTION             (-3)
#define POS_TARGET_FUNCTION_INDEX       (-2)
#define POS_ADDR_ARGUMENT_INDEX         (-1)

typedef struct InstrumentationPoint {
  std::string filename;
  std::vector<std::string> callstack;
  int id;
  Instruction *inst;
} InstrumentationPoint;

template <>
struct yaml::MappingTraits<InstrumentationPoint> {
  static void mapping(yaml::IO &io, InstrumentationPoint &instrumentation_point) {
    io.mapRequired("filename",  instrumentation_point.filename);
    io.mapRequired("callstack", instrumentation_point.callstack);
    io.mapRequired("id",        instrumentation_point.id);
  }
};

typedef std::vector<InstrumentationPoint> InstrumentationPointList;
LLVM_YAML_IS_SEQUENCE_VECTOR(InstrumentationPoint)

const char ViDeZZoGroupMutatorMissName[] = "GroupMutatorMiss";

namespace {

struct ViDeZZoInstrumentationLegacyPass: public ModulePass {
  static char ID;
  std::string ViDeZZoInstrumentationCallstack;
  InstrumentationPointList instrumentation_point_list;

  bool callstack_initialized;

  ViDeZZoInstrumentationLegacyPass(std::string ViDeZZoInstrumentationCallstack = "")
      : ModulePass(ID), ViDeZZoInstrumentationCallstack(ViDeZZoInstrumentationCallstack) {
    // Load the YAML file once
    auto error_or_file = MemoryBuffer::getFile(ViDeZZoInstrumentationCallstack);
    if (auto err = error_or_file.getError()) {
      report_fatal_error("Failed to open files");
    }

    yaml::Input yin((*error_or_file)->getBuffer());
    yin >> instrumentation_point_list;
    if (yin.error()) {
      report_fatal_error("Failed to deserialize yaml text");
    }
    errs() << "[+] Load " << instrumentation_point_list.size() << " instrumentation point(s)" << "\n";

    initializeViDeZZoInstrumentationLegacyPassPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override {
    LLVMContext * C = &(M.getContext());
    IRBuilder<> GlobalIRB(*C);

    // What to instrument
    FunctionCallee ViDeZZoGroupMutatorMiss = M.getOrInsertFunction(
      ViDeZZoGroupMutatorMissName, Type::getVoidTy(*C), GlobalIRB.getInt8Ty(), GlobalIRB.getInt64Ty());

    // Where to instrument
    bool instrumented = false;

    // Remove useless instrumentation points
    InstrumentationPointList filtered_instrumentation_point_list;
    StringRef current_source_file_name = sys::path::filename(M.getSourceFileName());
    for (InstrumentationPoint &ip : instrumentation_point_list) {
      if (current_source_file_name.compare(StringRef(ip.filename)) == 0)
        filtered_instrumentation_point_list.push_back(ip);
    }
    errs() << "[+] Select " << filtered_instrumentation_point_list.size() << " instrumentation point(s)" << "\n";
    if (filtered_instrumentation_point_list.size() == 0)
      return false;

    // For each instrumentation points
    for (InstrumentationPoint &ip : filtered_instrumentation_point_list) {
      // Match parent function
      for (auto &F : M) {
        StringRef function_name = F.getName();
        if (function_name.compare(StringRef(ip.callstack[ip.callstack.size() + POS_PARENT_FUNCTION])) != 0)
          continue;
        errs() << "[+] Find function " << function_name << "\n";

        // Find all call instructions here
        std::vector<Instruction*> calls;
        for (auto &BB : F) {
          for (auto &Inst : BB) {
            if(isa<CallInst>(Inst))
              calls.push_back(&Inst);
          }
        }
        errs() << "[+] Find " << calls.size() << " call instructions" << "\n";
        if (calls.size() == 0)
          continue;

        // Match target function
        ip.inst = NULL;
        StringRef called_function_name;
        int counter = 0;
        bool matched = false;

        for (Instruction *inst: calls) {
          CallInst *call = dyn_cast<CallInst>(inst);
          Function *fun = call->getCalledFunction();
          if (fun) {
            called_function_name = fun->getName();
          } else {
            LoadInst *load_inst = dyn_cast<LoadInst>(call->getCalledOperand());
            for (Use &U : load_inst->operands()) {
              Value *V = U.get();
              called_function_name = V->getName();
            }
          }
          if (called_function_name.startswith(StringRef(ip.callstack[ip.callstack.size() + POS_TARGET_FUNCTION])) == false)
            continue;

          if (counter != std::stoi(ip.callstack[ip.callstack.size() + POS_TARGET_FUNCTION_INDEX])) {
            counter++;
            continue;
          }
          // Bingo, we've found the target callsite
          ip.inst = inst;
          errs() << "[+] Find " << called_function_name << "." << counter << "\n";
          matched = true;
          break;
        }
        if (!matched) {
          errs() << "[+] Nothing found, last is " << called_function_name << "." << counter << "\n";
          continue;
        }

        // Instrument
        IRBuilder<> IRB(ip.inst);
        Value *GroupMutatorMissId = IRB.getInt8(ip.id);
        Value *GroupMutatorMissAddr = dyn_cast<CallInst>(ip.inst)->getArgOperand(
          std::stoi(ip.callstack[ip.callstack.size() + POS_ADDR_ARGUMENT_INDEX]));
        IRB.CreateCall(ViDeZZoGroupMutatorMiss, {
          IRB.CreateIntCast(GroupMutatorMissId, IRB.getInt8Ty(), false),
          IRB.CreateIntCast(GroupMutatorMissAddr, IRB.getInt64Ty(), false)});
        instrumented = true;
      }
    }
    return instrumented;
  }
};

} // end anonymous namespace

char ViDeZZoInstrumentationLegacyPass::ID = 0;

// TODO: enable opt and new pass manager
INITIALIZE_PASS_BEGIN(ViDeZZoInstrumentationLegacyPass,
                      "videzzo-instrumentation", "Instrumentation tool for ViDeZZo",
                      false, false)
// INITIALIZE_PASS_DEPENDENCY(WhateverYourPassDependencies)
INITIALIZE_PASS_END(ViDeZZoInstrumentationLegacyPass,
                    "videzzo-instrumentation", "Instrumentation tool for ViDeZZo",
                    false, false)

// createViDeZZoInstrumentation - Provide an entry point to create this pass.
ModulePass *llvm::createViDeZZoInstrumentationPass(StringRef ViDeZZoInstrumentationCallstack) {
  return new ViDeZZoInstrumentationLegacyPass(std:: string(ViDeZZoInstrumentationCallstack));
}