/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_KERNEL_CUM_MINMAX_CPU_KERNEL_H_
#define MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_KERNEL_CUM_MINMAX_CPU_KERNEL_H_

#include <vector>
#include <utility>
#include <string>
#include <memory>
#include <map>
#include "plugin/device/cpu/kernel/cpu_kernel.h"
#include "plugin/factory/ms_factory.h"

namespace mindspore {
namespace kernel {
constexpr auto kCummin = "Cummin";
constexpr auto kCummax = "Cummax";
constexpr auto kUnKnown = "UnKnown";
class CumMinMaxCpuKernelMod : public NativeCpuKernelMod {
 public:
  CumMinMaxCpuKernelMod() = default;
  explicit CumMinMaxCpuKernelMod(const std::string &kernel_type) : kernel_type_(kernel_type) {}
  ~CumMinMaxCpuKernelMod() override = default;

  bool Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
            const std::vector<KernelTensorPtr> &outputs) override;

  bool Resize(
    const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
    const std::vector<KernelTensorPtr> &outputs,
    const std::map<uint32_t, tensor::TensorPtr> &inputsOnHost = std::map<uint32_t, tensor::TensorPtr>()) override;

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
              const std::vector<AddressPtr> &outputs) override {
    return kernel_func_(this, inputs, outputs);
  }

  std::vector<KernelAttr> GetOpSupport() override;

 private:
  size_t GetRealIndex(size_t index);

  template <typename T, typename S>
  bool LaunchKernel(const std::vector<kernel::AddressPtr> &inputs, const std::vector<kernel::AddressPtr> &outputs);

  using CumMinMaxLaunchFunc = std::function<bool(CumMinMaxCpuKernelMod *, const std::vector<kernel::AddressPtr> &,
                                                 const std::vector<kernel::AddressPtr> &)>;
  static std::map<std::string, std::vector<std::pair<KernelAttr, CumMinMaxLaunchFunc>>> func_list_;
  CumMinMaxLaunchFunc kernel_func_;
  BaseOperatorPtr base_operator_;
  int64_t inner_size_{1};
  int64_t outer_size_{1};
  int64_t axis_size_{1};
  int64_t element_size_{1};     // Avoid repeated calculation
  int64_t axis_inner_size_{1};  // Avoid repeated calculation
  std::string kernel_type_{kUnKnown};
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_KERNEL_CUM_MINMAX_CPU_KERNEL_H_
