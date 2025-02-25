/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_INT8_DEPTH_TO_SPACE_INT8_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_INT8_DEPTH_TO_SPACE_INT8_H_

#include <vector>
#include "include/errorcode.h"
#include "nnacl/base/depth_to_space_base.h"
#include "nnacl/int8/depth_to_space_int8.h"
#include "nnacl/int8/quantize.h"
#include "src/runtime/kernel/cpu/fp32/depth_to_space_fp32.h"

namespace mindspore::kernel {
class DepthToSpaceInt8CPUKernel : public DepthToSpaceCPUKernel {
 public:
  DepthToSpaceInt8CPUKernel(OpParameter *parameter, const std::vector<lite::Tensor *> &inputs,
                            const std::vector<lite::Tensor *> &outputs, const lite::InnerContext *ctx)
      : DepthToSpaceCPUKernel(parameter, inputs, outputs, ctx) {}
  ~DepthToSpaceInt8CPUKernel() override;

  int Prepare() override;
  int Run() override;

 private:
  QuantArg *in_quant_arg_ = nullptr;
  QuantArg *out_quant_arg_ = nullptr;
};
}  // namespace mindspore::kernel

#endif  // MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_INT8_DEPTH_TO_SPACE_INT8_H_
