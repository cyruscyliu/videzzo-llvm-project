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
    void ResetTable() {
        memset(Table, 0, sizeof(Table));
        for (size_t i = 0; i < kStateMachineSize; i++) {
            Table[i].LastNode = NodeSize;
        }
    }

    // Clear accumulated state machines.
    void ResetTableAccumulated() {
        memset(TableAccumulated, 0, sizeof(TableAccumulated));
        for (size_t i = 0; i < kStateMachineSize; i++) {
            TableAccumulated[i].LastNode = NodeSize;
        }
    }

    // Return true if the byte is not saturated.
    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool UpdateState(uint8_t StateMachineId, size_t Node) {
        bool NodeRet, EdgeRet;
        NodeRet = UpdateNode(StateMachineId, Node);
        EdgeRet = UpdateEdge(StateMachineId, Node);
        return NodeRet && EdgeRet;
    }

    void PrintAccumulatedStatefulCoverage(bool PrintAllCounters) {
        // Because most state machines are empty,
        // we won't print all of them to be anonying.
        size_t i, j;
        for (i = 0; i < kStateMachineSize; i++) {
            // Check here.
            uint32_t acc = 0;
            for (j = 0; j < kNodeSize; j++)
                acc += GetAccumulatedNodeValue(i, j);
            if (acc == 0)
                continue;
            // Print then.
            Printf("==StateMachine %d==\n", i);
            Printf("====Node====\n");
            uint8_t v;
            for (j = 0; j < kNodeSize; j++) {
                v = GetAccumulatedNodeValue(i, j);
                if (PrintAllCounters)
                    v ? Printf("%02x", v) : Printf("--");
                else
                    v ? Printf("x"): Printf("-");
            }
            Printf("\n");
            // Go on edges.
            Printf("====Edge====\n");
            for (j = 0; j < kEdgeSize; j++) {
                if (j != 0 && j % kNodeSize == 0)
                    Printf("\n");
                v = GetAccumulatedEdgeValue(i ,j);
                if (PrintAllCounters)
                    v ?  Printf("%02x", v) : Printf("--");
                else
                    v ? Printf("x"): Printf("-");
            }
            Printf("\n");
        }
    }

    // Return size, which is the upper bound of features, of all bytes.
    size_t SizeInBytes() const { return kStateMachineSize * (kNodeSize + kEdgeSize); }

    template <class Callback>
    ATTRIBUTE_NO_SANITIZE_ALL
    void ForEachNonZeroByte(Callback CB) const {
        size_t i, j;
        StateMachine SM;
        for (i = 0; i < kStateMachineSize; i++) {
            SM = Table[i];
            for (j = 0; j < kNodeSize; j++) {
                if (GetNodeValue(i, j) == 0)
                    return;
                CB(i * (kNodeSize + kEdgeSize) + j, GetNodeValue(i, j));
            }
            for (j = 0; j < kEdgeSize; j++) {
                CB(i * (kNodeSize + kEdgeSize) + kNodeSize + j, GetEdgeValue(i, j));
            }
        }
    }

private:
    // Return true if the byte is not saturated.
    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool GetNodeValue(uint8_t StateMachineId, size_t Node) const {
        return Table[StateMachineId].NodeMap[Node];
    }

    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool GetAccumulatedNodeValue(uint8_t StateMachineId, size_t Node) const {
        return TableAccumulated[StateMachineId].NodeMap[Node];
    }

    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool GetEdgeValue(uint8_t StateMachineId, size_t Edge) const {
        return Table[StateMachineId].EdgeMap[Edge];
    }

    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool GetAccumulatedEdgeValue(uint8_t StateMachineId, size_t Edge) const {
        return TableAccumulated[StateMachineId].EdgeMap[Edge];
    }

    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool UpdateNode(uint8_t StateMachineId, size_t Node) {
        assert(Node < kNodeSize);
        // Update NodeMapAccumulated in any case
        if (TableAccumulated[StateMachineId].NodeMap[Node] < 0xFF) {
            TableAccumulated[StateMachineId].NodeMap[Node]++;
        }
        // Update NodeMap
        if (Table[StateMachineId].NodeMap[Node] < 0xFF) {
            Table[StateMachineId].NodeMap[Node]++;
            return true;
        }
        return false;
    }

    // Return true if the byte is not saturated.
    ATTRIBUTE_NO_SANITIZE_ALL
    inline bool UpdateEdge(uint8_t StateMachineId, size_t Node) {
        assert(Node < kNodeSize);
        size_t Edge;
        // Update EdgeMap
        // -------------> Node
        // |
        // v LastNode
        // Pos = LastNode * kNodeSize + Node
        if (Table[StateMachineId].LastNode == kNodeSize) {
            Table[StateMachineId].LastNode = Node;
        } else {
            Edge = Table[StateMachineId].LastNode * kNodeSize + Node;
            // Update EdgeMapAccumulated in any case
            if (TableAccumulated[StateMachineId].EdgeMap[Edge] < 0xFF)  {
                TableAccumulated[StateMachineId].EdgeMap[Edge]++;
            }
            if (Table[StateMachineId].EdgeMap[Edge] < 0xFF)  {
                Table[StateMachineId].EdgeMap[Edge]++;
                Table[StateMachineId].LastNode = Node;
                return true;
            }
        }
        return false;
    }
    ATTRIBUTE_ALIGNED(512) StateMachine Table[kStateMachineSize];
    ATTRIBUTE_ALIGNED(512) StateMachine TableAccumulated[kStateMachineSize];
};

}  // namespace fuzzer

#endif  // LLVM_FUZZER_STATE_TABLE_H
