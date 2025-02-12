/*
 * SPDX-License-Identifier: Apache-2.0
 */

// ATTENTION: The code in this file is highly EXPERIMENTAL.
// Adventurous users should note that the APIs will probably change.

#pragma once

#include "onnxoptimizer/pass.h"
#include "onnxoptimizer/passes/pass_util.h"

namespace ONNX_NAMESPACE {
namespace optimization {

struct FuseConsecutiveLogSoftmax final : public PredicateBasedPass {
  explicit FuseConsecutiveLogSoftmax()
      : PredicateBasedPass(PassType::Fuse, PassEfficiency::Complete,
                           PassOptimizationType::Compute) {}

  std::string getPassName() const override {
    return "fuse_consecutive_log_softmax";
  }

  bool patternMatchPredicate(Node* node) override {
    return CheckKind(node, kLog, 0, kSoftmax) &&
           node->input()->uses().size() == 1;
  }
  bool runTransform(Node* log_node, Graph& graph,
                    NodeDestroyType& destroy_current) override {
    Value* log_node_output = log_node->output();
    Node* softmax_node = PrevNode(log_node, 0);
    Node* log_softmax_node = graph.create(kLogSoftmax, 1);

    // log_softmax_node construction
    log_softmax_node->i_(kaxis, softmax_node->i(kaxis));
    log_softmax_node->addInput(softmax_node->input());
    log_softmax_node->insertBefore(softmax_node);
    log_softmax_node->output()->setSizes(log_node_output->sizes());
    log_softmax_node->output()->setElemType(log_node_output->elemType());

    const bool replacing_success =
        tryReplacingAllUsesWith(log_node, log_softmax_node);
    if (!replacing_success) {
      return false;
    }
    log_node->removeAllInputs();
    destroy_current = NodeDestroyType::DestroyTwo;
    return true;
  }
};

}  // namespace optimization
}  // namespace ONNX_NAMESPACE
