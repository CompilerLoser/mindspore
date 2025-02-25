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

#include "src/runtime/kernel/cpu/fp16/transpose_fp16.h"
#include "src/kernel_registry.h"
#include "nnacl/fp16/pack_fp16.h"
#include "nnacl/fp16/transpose_fp16.h"

using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_OK;
using mindspore::schema::PrimitiveType_Transpose;

namespace mindspore::kernel {
int TransposeFp16CPUKernel::ReSize() {
  auto ret = TransposeBaseCPUKernel::ReSize();
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Do transpose resize failed.";
    return ret;
  }
  thread_num_ = (!opt_run_ && param_->num_axes_ <= DIMENSION_6D) ? 1 : thread_num_;
  return RET_OK;
}

int TransposeFp16CPUKernel::DoTransposeSingleThread() {
  if (opt_run_ || param_->num_axes_ > DIMENSION_6D) {
    return DoTransposeMultiThread(0);
  }
  return DoTransposeFp16(static_cast<const float16_t *>(in_data_), static_cast<float16_t *>(out_data_), out_shape_,
                         param_);
}

int TransposeFp16CPUKernel::DoTransposeMultiThread(int task_id) {
  if (opt_run_) {
    PackNHWCToNCHWFp16(in_data_, out_data_, opt_param_[FIRST_INPUT], opt_param_[SECOND_INPUT], opt_param_[THIRD_INPUT],
                       task_id, thread_num_);
    return RET_OK;
  }
  TransposeDimsFp16(static_cast<const float16_t *>(in_data_), static_cast<float16_t *>(out_data_), out_shape_, param_,
                    task_id, thread_num_);
  return RET_OK;
}

REG_KERNEL(kCPU, kNumberTypeFloat16, PrimitiveType_Transpose, LiteKernelCreator<TransposeFp16CPUKernel>)
}  // namespace mindspore::kernel
