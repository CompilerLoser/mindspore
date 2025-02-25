/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_LITE_INCLUDE_CONTEXT_H_
#define MINDSPORE_LITE_INCLUDE_CONTEXT_H_
#include <string>
#include "include/ms_tensor.h"
#include "include/lite_utils.h"
#include "include/lite_types.h"

namespace mindspore::lite {
/// \brief CpuDeviceInfo defined for CPU's configuration information.
typedef struct CpuDeviceInfo {
  bool enable_float16_ = false; /**< prior enable float16 inference */
  CpuBindMode cpu_bind_mode_ = MID_CPU;
} CpuDeviceInfo;

/// \brief GpuDeviceInfo defined for GPU's configuration information.
typedef struct GpuDeviceInfo {
  bool enable_float16_ = false; /**< prior enable float16 inference */
  uint32_t gpu_device_id_ = 0;
  int rank_id_ = 0;
  int group_size_ = 0;
  bool enable_gl_texture_ = false; /**<enable sharing OpenGL texture with OpenCL */
  void *gl_context_ = nullptr;
  void *gl_display_ = nullptr;
} GpuDeviceInfo;

/// \brief NpuDeviceInfo defined for NPU's configuration information.
typedef struct NpuDeviceInfo {
  int frequency_ = 3; /**< npu frequency inference, low 1, medium 2, high 3, extreme 4, other values will be set to 3 */
} NpuDeviceInfo;

/// \brief AscendDeviceInfo defined for Ascend's configuration information.
typedef struct AscendDeviceInfo {
  uint32_t device_id_ = 0;
  std::string batch_size_;
  std::string image_size_;
} AscendDeviceInfo;
/// \brief DeviceInfo defined for backend's configuration information.
struct DeviceInfo {
  CpuDeviceInfo cpu_device_info_;
  GpuDeviceInfo gpu_device_info_;
  NpuDeviceInfo npu_device_info_;
  AscendDeviceInfo ascend_device_info_;
};

/// \brief DeviceContext defined for holding backend's configuration information.
struct DeviceContext {
  DeviceType device_type_ = DT_CPU;
  DeviceInfo device_info_;
  std::string provider_{};
  std::string provider_device_{};
  AllocatorPtr allocator_ = nullptr;
};

/// \brief Context defined for holding environment variables during runtime.
struct Context {
  String vendor_name_;
  int thread_num_ = 2; /**< thread number config for thread pool */
  int inter_op_parallel_num_ = 1;
  bool enable_parallel_ = false;
  Vector<int> affinity_core_list_; /**< explicitly specify the core to be bound. priority use affinity core list */
  AllocatorPtr allocator = nullptr;
#ifndef NOT_USE_STL
  DeviceContextVector device_list_ = {{DT_CPU, {{false, MID_CPU}}}};
#else
  DeviceContextVector device_list_;
#endif  // NOT_USE_STL
  DelegatePtr delegate = nullptr;
  bool float_mode = false; /**< convert full quant model to float model */
};
}  // namespace mindspore::lite
#endif  // MINDSPORE_LITE_INCLUDE_CONTEXT_H_
