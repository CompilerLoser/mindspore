/**
 * Copyright 2020-2022 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "plugin/device/gpu/kernel/arrays/transpose_gpu_kernel.h"
namespace mindspore {
namespace kernel {
MS_REG_GPU_KERNEL_ONE(Transpose, KernelAttr().AddInputAttr(kNumberTypeBool).AddOutputAttr(kNumberTypeBool),
                      TransposeFwdGpuKernelMod, bool)
MS_REG_GPU_KERNEL_ONE(Transpose, KernelAttr().AddInputAttr(kNumberTypeFloat64).AddOutputAttr(kNumberTypeFloat64),
                      TransposeFwdGpuKernelMod, double)
MS_REG_GPU_KERNEL_ONE(Transpose, KernelAttr().AddInputAttr(kNumberTypeFloat32).AddOutputAttr(kNumberTypeFloat32),
                      TransposeFwdGpuKernelMod, float)
MS_REG_GPU_KERNEL_ONE(Transpose, KernelAttr().AddInputAttr(kNumberTypeFloat16).AddOutputAttr(kNumberTypeFloat16),
                      TransposeFwdGpuKernelMod, half)
MS_REG_GPU_KERNEL_ONE(Transpose, KernelAttr().AddInputAttr(kNumberTypeInt32).AddOutputAttr(kNumberTypeInt32),
                      TransposeFwdGpuKernelMod, int)
MS_REG_GPU_KERNEL_ONE(Transpose, KernelAttr().AddInputAttr(kNumberTypeInt64).AddOutputAttr(kNumberTypeInt64),
                      TransposeFwdGpuKernelMod, int64_t)
}  // namespace kernel
}  // namespace mindspore
