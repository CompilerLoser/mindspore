/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include "src/kernel_exec.h"
#include <algorithm>
#include "src/tensor.h"
#include "src/common/utils.h"
#include "src/common/version_manager.h"

namespace mindspore::kernel {
using mindspore::lite::RET_ERROR;
using mindspore::lite::RET_OK;

bool KernelExec::IsReady(const std::vector<lite::Tensor *> &scope_tensors) {
  MS_ASSERT(kernel_ != nullptr);
  auto &in_tensors = this->in_tensors();
  return std::all_of(in_tensors.begin(), in_tensors.end(), [&](lite::Tensor *in_tensor) {
    if (IsContain(scope_tensors, in_tensor)) {
      return in_tensor->IsReady();
    } else {
      return true;
    }
  });
}

void KernelExec::InitOutTensorInitRefCount(const std::vector<KernelExec *> *mask_kernels) {
  for (auto *tensor : this->out_tensors()) {
    MS_ASSERT(tensor != nullptr);
    int init_ref_count = 0;
    for (auto *post_kernel : this->out_kernels_) {
      if ((mask_kernels == nullptr) ||
          std::find(mask_kernels->begin(), mask_kernels->end(), post_kernel) != mask_kernels->end()) {
        auto &post_in_tensors = post_kernel->in_tensors();
        init_ref_count += std::count_if(
          post_in_tensors.begin(), post_in_tensors.end(),
          [&tensor](const lite::Tensor *post_kernel_in_tensor) { return post_kernel_in_tensor == tensor; });
      }
    }
    tensor->set_init_ref_count(init_ref_count);
  }
}

std::string KernelExec::ToString() const {
  std::ostringstream oss;
  oss << "KernelExec: " << this->name();
  oss << ", Type: " << this->type_str() << std::endl;
  oss << this->in_tensors().size() << " InputTensors:" << std::endl;
  for (auto tensor : in_tensors()) {
    oss << tensor->ToString() << std::endl;
  }
  oss << this->out_tensors().size() << " OutputTensors:" << std::endl;
  for (auto tensor : out_tensors()) {
    oss << tensor->ToString() << std::endl;
  }
  oss << this->in_kernels_.size() << " InputKernels: ";
  for (auto in_kernel : in_kernels_) {
    oss << in_kernel->name() << ", ";
  }
  oss << std::endl << this->out_kernels_.size() << " OutputKernels: ";
  for (auto out_kernel : out_kernels_) {
    oss << out_kernel->name() << ", ";
  }
  return oss.str();
}

int KernelExec::DoExecute() {
  auto ret = kernel_->Execute();
  if ((ret == lite::RET_OK) && (desc_.provider != kBuiltin)) {
    for (auto *output : out_tensors()) {
      MS_ASSERT(output != nullptr);
      output->ResetRefCount();
    }
    for (auto &in_tensor : in_tensors()) {
      MS_ASSERT(in_tensor != nullptr);
      in_tensor->DecRefCount();
    }
  }
  return ret;
}

void KernelExec::RepalceKernel(std::shared_ptr<Kernel> kernel) {
  if (desc_.provider == kBuiltin) {
    std::static_pointer_cast<LiteKernel>(kernel_)->set_parameter(nullptr);  // set nullptr, don't release op_parameter
    kernel_.reset();
    kernel_ = kernel;
  }
}
}  // namespace mindspore::kernel
