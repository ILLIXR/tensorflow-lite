/* Copyright 2021 The OpenXLA Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef MLIR_HLO_TRANSFORMS_PASSES_H
#define MLIR_HLO_TRANSFORMS_PASSES_H

#include <functional>
#include <memory>

#include "mlir/Pass/Pass.h"

namespace mlir {
class ModuleOp;
class MLIRContext;
class ConversionTarget;
class DialectRegistry;
class PassManager;

namespace func {
class FuncOp;
}  // namespace func
namespace bufferization {
class BufferizeTypeConverter;
}  // namespace bufferization

using BufferizeDialectsCallback = std::function<void(DialectRegistry&)>;
using BufferizePatternsCallback = std::function<void(
    ConversionTarget&, MLIRContext*, bufferization::BufferizeTypeConverter*,
    RewritePatternSet*)>;

//===----------------------------------------------------------------------===//
// Passes
//===----------------------------------------------------------------------===//

#define GEN_PASS_DECL_FINALBUFFERIZEPASS
#define GEN_PASS_DECL_PROPAGATESTATICSHAPESTOKERNELPASS
#define GEN_PASS_DECL_TILELOOPSPASS
#define GEN_PASS_DECL_GENERICHOSTTOLLVMPASS
#define GEN_PASS_DECL_VECTORIZECOPYPASS
#include "transforms/passes.h.inc"

// Pass to lower index cast on tensors to tensor dialect.
std::unique_ptr<OperationPass<func::FuncOp>> createLowerIndexCastPass();

// Pass to tranform compute computations (hlo and linalg) on values to their
// corresponding counterparts on buffers. Also bufferizes function signatures.
std::unique_ptr<OperationPass<ModuleOp>> createComputeOpAndFuncBufferizePass();

// Pass to tranform computations on values to their corresponding parts on
// buffers.
std::unique_ptr<OperationPass<ModuleOp>> createFinalBufferizePass();

std::unique_ptr<OperationPass<ModuleOp>> createFinalBufferizePass(
    uint64_t alignment, BufferizeDialectsCallback dc = {},
    BufferizePatternsCallback pc = {});

// Pass to propagate static shapes to kernel, reducing the kernel arguments
// from a flattened memref to a single pointer. The pointer is converted to
// `pointer_type`, if provided.
std::unique_ptr<OperationPass<ModuleOp>>
createPropagateStaticShapesToKernelPass(Type pointerType = {});

// Creates a pass for collapsing multidimensional parallel loops into 1D loops.
std::unique_ptr<OperationPass<>> createCollapseParallelLoopsTo1DPass();

// Creates a TileLoopsPass with tiles sizes provided through `tile_sizes`
// and unroll factors provided through `unroll_factors`.
std::unique_ptr<OperationPass<func::FuncOp>> createTileLoopsPass(
    ArrayRef<int64_t> tileSizes = {}, ArrayRef<int64_t> unrollFactors = {});

// Detensorizes loop-carried variables and block arguments of scf.while, scf.for
// and scf.if.
std::unique_ptr<OperationPass<func::FuncOp>> createDetensorizeScfOpsPass();

/// Pass to remove redundant `memref.copy` ops.
std::unique_ptr<OperationPass<func::FuncOp>> createNaiveCopyRemovalPass();

/// Pass to vectorize `memref.copy`.
std::unique_ptr<OperationPass<func::FuncOp>> createVectorizeCopyPass();

namespace hlo {
std::unique_ptr<OperationPass<ModuleOp>> createOneShotBufferizePass();

std::unique_ptr<OperationPass<ModuleOp>> createGenericHostToLLVMPass(
    const GenericHostToLLVMPassOptions& options = {});

std::unique_ptr<OperationPass<func::FuncOp>> createUnbufferizePass();
std::unique_ptr<OperationPass<func::FuncOp>> createAllocToArgPass();

// Unrolls scf.for loops with static iteration count no larger than 8.
std::unique_ptr<Pass> createUnrollLoopsPass();

#define GEN_PASS_REGISTRATION
#include "transforms/passes.h.inc"

}  // namespace hlo
}  // namespace mlir

#endif  // MLIR_HLO_TRANSFORMS_PASSES_H
