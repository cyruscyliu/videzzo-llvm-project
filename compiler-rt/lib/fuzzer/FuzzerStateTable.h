//===- FuzzerStateTable.h - Internal header for the Fuzzer -----*- C++ -* ===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// StateTable.
//===----------------------------------------------------------------------===//

#ifndef LLVM_FUZZER_STATE_TABLE_H
#define LLVM_FUZZER_STATE_TABLE_H

#include "FuzzerPlatform.h"
#include "FuzzerIO.h"
#include <cstdint>

using namespace std;

namespace fuzzer {

#define StateMachineSize (1 << 8) // 256
#define NodeSize (1 << 6) // 64
#define EdgeSize ((1 << 6) * ((1 << 6) - 1)) // 64 * 63

// A state machine.
struct StateMachine {
    size_t LastNode;
    uint8_t NodeMap[NodeSize];
    uint8_t EdgeMap[EdgeSize];
};

// A state table containing kStateSize state machines.
struct StateTable {
    static const size_t kStateMachineSize = StateMachineSize;
    static const size_t kNodeSize = NodeSize;
    static const size_t kEdgeSize = EdgeSize;

public:
    // Clear state machines.
    void Reset() {
        memset(Table, 0, sizeof(Table));
        for (size_t i = 0; i < kStateMachineSize; i++) {
            Table[i].LastNode = NodeSize;
        }
    }

    // Return true if the byte is not saturated.
    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool UpdateState(uint8_t StateMachineId, size_t Node) {
        return true;
    }

    void Print(bool PrintAllCounters) {
    }

    // Return size, which is the upper bound of features, of all bytes.
    size_t SizeInBytes() const { return kStateMachineSize * (kNodeSize + kEdgeSize); }

    template <class Callback>
    ATTRIBUTE_NO_SANITIZE_ALL
    void ForEachNonZeroByte(Callback CB) const {
    }

private:
    // Return true if the byte is not saturated.
    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool GetNodeValue(uint8_t StateMachineId, size_t Node) const {
        return Table[StateMachineId].NodeMap[Node];
    }

    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool GetEdgeValue(uint8_t StateMachineId, size_t Edge) const {
        return Table[StateMachineId].EdgeMap[Edge];
    }

    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool UpdateNode(uint8_t StateMachineId, size_t Node) {
        return false;
    }

    // Return true if the byte is not saturated.
    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool UpdateEdge(uint8_t StateMachineId, size_t Node) {
        return false;
    }
    ATTRIBUTE_ALIGNED(512) StateMachine Table[kStateMachineSize];
};

}  // namespace fuzzer

#endif  // LLVM_FUZZER_STATE_TABLE_H
