//===- ViDeZZoInstrumentation.h - The -videzzoinstrumentation pass     ----===//
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

#ifndef LLVM_TRANSFORMS_UTILS_VIDEZZO_INSTRUMENTATION_H
#define LLVM_TRANSFORMS_UTILS_VIDEZZO_INSTRUMENTATION_H

#include "llvm/Support/YAMLTraits.h"
#include "llvm/IR/Instructions.h"

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

#endif /* LLVM_TRANSFORMS_UTILS_VIDEZZO_INSTRUMENTATION_H */
