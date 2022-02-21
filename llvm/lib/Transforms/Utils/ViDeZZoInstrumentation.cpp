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
#include "llvm/InitializePasses.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;

namespace {

struct ViDeZZoInstrumentationLegacyPass: public FunctionPass {
  static char ID;
  std::string ViDeZZoInstrumentationCallstack;

  ViDeZZoInstrumentationLegacyPass(std::string ViDeZZoInstrumentationCallstack = "")
      : FunctionPass(ID), ViDeZZoInstrumentationCallstack(ViDeZZoInstrumentationCallstack) {
    initializeViDeZZoInstrumentationLegacyPassPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
      printf("you find me!: %s\n", ViDeZZoInstrumentationCallstack.c_str());
      return false;
  }
};

} // end anonymous namespace

char ViDeZZoInstrumentationLegacyPass::ID = 0;

INITIALIZE_PASS_BEGIN(ViDeZZoInstrumentationLegacyPass,
                      "videzzoinstrumentation", "Instrumentation tool for ViDeZZo",
                      false, false)
// INITIALIZE_PASS_DEPENDENCY(WhateverYourPassDependencies)
INITIALIZE_PASS_END(ViDeZZoInstrumentationLegacyPass,
                    "videzzoinstrumentation", "Instrumentation tool for ViDeZZo",
                    false, false)

// createViDeZZoInstrumentation - Provide an entry point to create this pass.
FunctionPass *llvm::createViDeZZoInstrumentationPass(StringRef ViDeZZoInstrumentationCallstack) {
  return new ViDeZZoInstrumentationLegacyPass(std:: string(ViDeZZoInstrumentationCallstack));
}
