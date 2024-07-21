/***************************************************************************************************
 * Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************************************/

/*! \file
  \brief 
    Defines a GEMM with Broadcast based on an existing UniversalGemm kernel.

*/

#pragma once

#include "cutlass/cutlass.h"

#include "cutlass/conv/kernel/default_deconv3d.h"
#include "cutlass/conv/kernel/implicit_gemm_convolution_with_fused_epilogue.h"

#include "cutlass/epilogue/threadblock/default_epilogue_with_broadcast.h"
#include "cutlass/epilogue/threadblock/epilogue_with_broadcast.h"

/////////////////////////////////////////////////////////////////////////////////////////////////

namespace cutlass {
namespace conv {
namespace kernel {

/////////////////////////////////////////////////////////////////////////////////////////////////

template <
  typename ElementA,
  typename LayoutA,
  typename ElementB,
  typename LayoutB,
  typename ElementC,
  typename LayoutC,
  typename ElementAccumulator,
  typename OperatorClass,
  typename ArchTag,
  typename ThreadblockShape,
  typename WarpShape,
  typename InstructionShape,
  typename EpilogueOutputOp,
  typename ThreadblockSwizzle,
  int Stages,
  typename MathOperatorTag,
  conv::IteratorAlgorithm IteratorAlgorithm = IteratorAlgorithm::kOptimized,
  conv::StrideSupport StrideSupport = StrideSupport::kStrided,
  /// Access granularity of A matrix in units of elements
  int AlignmentA = 128 / cutlass::sizeof_bits<ElementA>::value,
  /// Access granularity of B matrix in units of elements
  int AlignmentB = 128 / cutlass::sizeof_bits<ElementB>::value
>
struct DefaultDeconv3dWithBroadcast {

  using ImplicitGemmBase = typename DefaultDeconv3d<
    ElementA, LayoutA,
    ElementB, LayoutB,
    ElementC, LayoutC,
    ElementAccumulator,
    OperatorClass,
    ArchTag,
    ThreadblockShape,
    WarpShape,
    InstructionShape,
    EpilogueOutputOp,
    ThreadblockSwizzle,
    Stages,
    MathOperatorTag,
    IteratorAlgorithm,
    StrideSupport
  >::Kernel;

  // Define epilogue
  using Epilogue = typename cutlass::conv::kernel::detail::DefaultConvEpilogueWithBroadcastTensorOp<
    ArchTag,
    typename ImplicitGemmBase::Epilogue::Shape,
    typename ImplicitGemmBase::Epilogue::WarpMmaOperator,
    ImplicitGemmBase::Epilogue::kPartitionsK,
    ElementC,
    typename EpilogueOutputOp::ElementT,
    typename EpilogueOutputOp::ElementVector,
    EpilogueOutputOp,
    ImplicitGemmBase::Epilogue::kElementsPerAccess
  >::Epilogue;

  // Define the kernel
  using Kernel = cutlass::conv::kernel::ImplicitGemmConvolutionWithFusedEpilogue<
    typename ImplicitGemmBase::Mma,
    Epilogue,
    ThreadblockSwizzle,
    conv::Operator::kDeconv,
    Conv3dProblemSize
  >;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                            OpClassSimt convolutions
/////////////////////////////////////////////////////////////////////////////////////////////////
/// Defines a kernel for Deconv3d specialization for Analytic IteratorAlgorithm,
/// multi-stage pipeline, and FFMA-based mainloop for SM80

template <
  typename ElementA,
  typename LayoutA,
  typename ElementB,
  typename LayoutB,
  typename ElementC,
  typename LayoutC,
  typename ElementAccumulator,
  typename ArchTag,
  typename ThreadblockShape,
  typename WarpShape,
  typename InstructionShape,
  typename EpilogueOutputOp,
  typename ThreadblockSwizzle,
  int Stages,
  typename MathOperatorTag,
  conv::IteratorAlgorithm IteratorAlgorithm,
  int AlignmentA,
  int AlignmentB
>
struct DefaultDeconv3dWithBroadcast <
  ElementA,
  LayoutA,
  ElementB,
  LayoutB,
  ElementC,
  LayoutC,
  ElementAccumulator,
  arch::OpClassSimt,
  ArchTag,
  ThreadblockShape,
  WarpShape,
  InstructionShape,
  EpilogueOutputOp,
  ThreadblockSwizzle,
  Stages,
  MathOperatorTag,
  IteratorAlgorithm,
  conv::StrideSupport::kUnity,
  AlignmentA,
  AlignmentB
> {

  using ImplicitGemmBase = typename DefaultDeconv3d<
    ElementA, LayoutA,
    ElementB, LayoutB,
    ElementC, LayoutC,
    ElementAccumulator,
    arch::OpClassSimt,
    ArchTag,
    ThreadblockShape,
    WarpShape,
    InstructionShape,
    EpilogueOutputOp,
    ThreadblockSwizzle,
    Stages,
    MathOperatorTag,
    IteratorAlgorithm,
    conv::StrideSupport::kUnity
  >::Kernel;

  // Define epilogue
  using Epilogue = typename cutlass::conv::kernel::detail::DefaultConvEpilogueWithBroadcastSimt<
    ArchTag,
    typename ImplicitGemmBase::Epilogue::Shape,
    typename ImplicitGemmBase::Epilogue::WarpMmaOperator,
    ElementC,
    typename EpilogueOutputOp::ElementT,
    typename EpilogueOutputOp::ElementVector,
    EpilogueOutputOp,
    ImplicitGemmBase::Epilogue::kElementsPerAccess
  >::Epilogue;

  // Define the kernel
  using Kernel = cutlass::conv::kernel::ImplicitGemmConvolutionWithFusedEpilogue<
    typename ImplicitGemmBase::Mma,
    Epilogue,
    ThreadblockSwizzle,
    conv::Operator::kDeconv,
    Conv3dProblemSize
  >;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

template <
  typename ElementA,
  typename LayoutA,
  typename ElementB,
  typename LayoutB,
  typename ElementC,
  typename LayoutC,
  typename ElementAccumulator,
  typename ArchTag,
  typename ThreadblockShape,
  typename WarpShape,
  typename InstructionShape,
  typename EpilogueOutputOp,
  typename ThreadblockSwizzle,
  int Stages,
  typename MathOperatorTag,
  conv::IteratorAlgorithm IteratorAlgorithm,
  int AlignmentA,
  int AlignmentB
>
struct DefaultDeconv3dWithBroadcast <
  ElementA,
  LayoutA,
  ElementB,
  LayoutB,
  ElementC,
  LayoutC,
  ElementAccumulator,
  arch::OpClassSimt,
  ArchTag,
  ThreadblockShape,
  WarpShape,
  InstructionShape,
  EpilogueOutputOp,
  ThreadblockSwizzle,
  Stages,
  MathOperatorTag,
  IteratorAlgorithm,
  conv::StrideSupport::kStrided,
  AlignmentA,
  AlignmentB
> {

  using ImplicitGemmBase = typename DefaultDeconv3d<
    ElementA, LayoutA,
    ElementB, LayoutB,
    ElementC, LayoutC,
    ElementAccumulator,
    arch::OpClassSimt,
    ArchTag,
    ThreadblockShape,
    WarpShape,
    InstructionShape,
    EpilogueOutputOp,
    ThreadblockSwizzle,
    Stages,
    MathOperatorTag,
    IteratorAlgorithm,
    conv::StrideSupport::kStrided
  >::Kernel;

  // Define epilogue
  using Epilogue = typename cutlass::conv::kernel::detail::DefaultConvEpilogueWithBroadcastSimtStridedDgrad<
    ArchTag,
    typename ImplicitGemmBase::Epilogue::Shape,
    typename ImplicitGemmBase::Epilogue::WarpMmaOperator,
    ElementC,
    typename EpilogueOutputOp::ElementT,
    typename EpilogueOutputOp::ElementVector,
    EpilogueOutputOp,
    ImplicitGemmBase::Epilogue::kElementsPerAccess
  >::Epilogue;

  // Define the kernel
  using Kernel = cutlass::conv::kernel::ImplicitGemmConvolutionWithFusedEpilogue<
    typename ImplicitGemmBase::Mma,
    Epilogue,
    ThreadblockSwizzle,
    conv::Operator::kDeconv,
    Conv3dProblemSize
  >;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

}  // namespace kernel
}  // namespace conv
}  // namespace cutlass

/////////////////////////////////////////////////////////////////////////////////////////////////
