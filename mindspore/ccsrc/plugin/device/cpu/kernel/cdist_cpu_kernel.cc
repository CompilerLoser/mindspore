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

#include "plugin/device/cpu/kernel/cdist_cpu_kernel.h"
#include <utility>
#include <algorithm>
#include "plugin/device/cpu/kernel/nnacl/op_base.h"
namespace mindspore {
namespace kernel {
namespace {
constexpr size_t kCdistInputDimsMin = 2;

const std::vector<KernelAttr> kernel_attr = {
  {KernelAttr().AddInputAttr(kNumberTypeFloat32).AddInputAttr(kNumberTypeFloat32).AddOutputAttr(kNumberTypeFloat32)}};
}  // namespace

template <typename T>
void CdistZeroNormalcompute(const void *in_data0, const void *in_data1, void *out_data, int64_t m, float p) {
  const auto *a = reinterpret_cast<const T *>(in_data0);
  const auto *b = reinterpret_cast<const T *>(in_data1);
  auto *c = reinterpret_cast<T *>(out_data);
  T result = 0;
  for (int64_t i = 0; i < m; i++) {
    auto x = std::abs(a[i] - b[i]);
    result += std::min(std::ceil(x), 1.0f);
  }
  *c = result;
}

template <typename T>
void CdistOneNormalcompute(const void *in_data0, const void *in_data1, void *out_data, int64_t m, float p) {
  const auto *a = reinterpret_cast<const T *>(in_data0);
  const auto *b = reinterpret_cast<const T *>(in_data1);
  auto *c = reinterpret_cast<T *>(out_data);
  T result = 0;
  for (int64_t i = 0; i < m; i++) {
    auto x = std::abs(a[i] - b[i]);
    result += x;
  }
  *c = result;
}

template <typename T>
void CdistTwoNormalcompute(const void *in_data0, const void *in_data1, void *out_data, int64_t m, float p) {
  const auto *a = reinterpret_cast<const T *>(in_data0);
  const auto *b = reinterpret_cast<const T *>(in_data1);
  auto *c = reinterpret_cast<T *>(out_data);
  T result = 0;
  for (int64_t i = 0; i < m; i++) {
    auto x = std::abs(a[i] - b[i]);
    result += x * x;
  }
  result = std::sqrt(result);
  *c = result;
}

template <typename T>
void CdistPNormalcompute(const void *in_data0, const void *in_data1, void *out_data, int64_t m, float p) {
  const auto *a = reinterpret_cast<const T *>(in_data0);
  const auto *b = reinterpret_cast<const T *>(in_data1);
  auto *c = reinterpret_cast<T *>(out_data);
  T result = 0;
  for (int64_t i = 0; i < m; i++) {
    auto x = std::abs(a[i] - b[i]);
    result += std::pow(x, p);
  }
  *c = std::pow(result, 1.0 / p);
}

template <typename T>
void CdistInfNormalcompute(const void *in_data0, const void *in_data1, void *out_data, int64_t m, float p) {
  const auto *a = reinterpret_cast<const T *>(in_data0);
  const auto *b = reinterpret_cast<const T *>(in_data1);
  auto *c = reinterpret_cast<T *>(out_data);
  T result = 0;
  for (int64_t i = 0; i < m; i++) {
    auto x = std::abs(a[i] - b[i]);
    result = std::max(result, x);
  }
  *c = result;
}

template <typename T>
void CdistCpuKernelMod::InitFunc(float p) {
  kernel_func_ = &CdistCpuKernelMod::LaunchKernel<T>;

  if (p == 0.0) {
    dist_func_ = CdistZeroNormalcompute<T>;
  } else if (p == 1.0) {
    dist_func_ = CdistOneNormalcompute<T>;
  } else if (p == 2.0) {
    dist_func_ = CdistTwoNormalcompute<T>;
  } else if (std::isinf(p)) {
    dist_func_ = CdistInfNormalcompute<T>;
  } else {
    dist_func_ = CdistPNormalcompute<T>;
  }
}

bool CdistCpuKernelMod::Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
                             const std::vector<KernelTensorPtr> &outputs) {
  auto kernel_ptr = std::dynamic_pointer_cast<ops::Cdist>(base_operator);
  if (kernel_ptr == nullptr) {
    MS_LOG(ERROR) << "cast Cdist ops failed!";
    return false;
  }
  kernel_name_ = kernel_ptr->name();
  p_ = kernel_ptr->get_p();

  auto input_type_id = inputs[0]->GetDtype();
  switch (input_type_id) {
    case kNumberTypeFloat32:
      InitFunc<float>(p_);
      break;
    default:
      MS_LOG(ERROR) << "cdist kernel does not support " << TypeIdToString(input_type_id);
      return false;
  }
  return true;
}

bool CdistCpuKernelMod::Resize(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
                               const std::vector<KernelTensorPtr> &outputs,
                               const std::map<uint32_t, tensor::TensorPtr> &inputsOnHost) {
  if (!NativeCpuKernelMod::Resize(base_operator, inputs, outputs, inputsOnHost)) {
    MS_LOG(ERROR) << kernel_name_ << " reinit failed.";
    return false;
  }
  std::vector<int64_t> in_shape0 = inputs[0]->GetShapeVector();
  std::vector<int64_t> in_shape1 = inputs[1]->GetShapeVector();
  auto in_shape_size = in_shape0.size();
  if (in_shape1.size() != in_shape_size || in_shape_size < kCdistInputDimsMin) {
    MS_LOG(ERROR) << "invalid input shape, input0 shape size " << in_shape_size << ", input1 shape size "
                  << in_shape1.size() << ", kernel_name_ " << kernel_name_;
    return false;
  }
  batch_ = 0;
  for (size_t i = 0; i < in_shape_size - kCdistInputDimsMin; i++) {
    batch_ += in_shape0[i];
  }
  batch_ = (batch_ <= 0) ? 1 : batch_;

  r0_ = in_shape0[in_shape_size - 2];
  m_ = in_shape0[in_shape_size - 1];
  r1_ = in_shape1[in_shape_size - 2];

  thread_num_ = std::min(static_cast<size_t>(batch_), pool_->GetKernelThreadNum());

  return true;
}

template <typename T>
bool CdistCpuKernelMod::LaunchKernel(int64_t start, int64_t end) {
  const auto *in_data0 = reinterpret_cast<T *>(in_data0_) + start * r0_ * m_;
  const auto *in_data1 = reinterpret_cast<T *>(in_data1_) + start * r1_ * m_;
  auto *out_data = reinterpret_cast<T *>(out_data_) + start * r0_ * r1_;

  for (int64_t b_i = 0; b_i < end - start; b_i++) {
    for (int64_t p_i = 0; p_i < r0_; p_i++) {
      auto in_data_tmp1 = in_data1;
      for (int64_t r_i = 0; r_i < r1_; r_i++) {
        dist_func_(in_data0, in_data1, &(out_data[r_i]), m_, p_);
        in_data_tmp1 = in_data_tmp1 + m_;
      }
      in_data0 = in_data0 + m_;
      out_data = out_data + r1_;
    }
    in_data1 = in_data1 + r1_ * m_;
  }

  return true;
}

std::vector<KernelAttr> CdistCpuKernelMod::GetOpSupport() { return kernel_attr; }

bool CdistCpuKernelMod::DoLaunch(int task_id) {
  auto batch_per_thread = UP_DIV(batch_, thread_num_);
  int64_t start = batch_per_thread * task_id;
  int64_t end = start + batch_per_thread;
  end = std::min(end, batch_);
  return kernel_func_(this, start, end);
}

int CdistRun(void *cdata, int task_id, float lhs_scale, float rhs_scale) {
  auto cdist_kernel = reinterpret_cast<CdistCpuKernelMod *>(cdata);
  if (!cdist_kernel->DoLaunch(task_id)) {
    MS_LOG(ERROR) << "cdist_kernel DoLaunch failed, task_id:" << task_id;
    return -1;
  }
  return 0;
}

bool CdistCpuKernelMod::Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
                               const std::vector<AddressPtr> &outputs) {
  in_data0_ = inputs[0]->addr;
  in_data1_ = inputs[1]->addr;
  out_data_ = outputs[0]->addr;
  int ret = pool_->ParallelLaunch(CdistRun, this, thread_num_);
  if (ret != 0) {
    MS_LOG(ERROR) << "CdistCpuKernelMod ParallelLaunch failed, error_code[" << ret << "]";
    return false;
  }
  return true;
}

MS_KERNEL_FACTORY_REG(NativeCpuKernelMod, Cdist, CdistCpuKernelMod);
}  // namespace kernel
}  // namespace mindspore
