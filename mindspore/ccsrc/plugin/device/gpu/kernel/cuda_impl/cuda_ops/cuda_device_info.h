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

#ifndef MINDSPORE_CCSRC_PLUGIN_DEVICE_GPU_KERNEL_CUDA_IMPL_CUDA_OPS_CUDA_DEVICE_INFO_H_
#define MINDSPORE_CCSRC_PLUGIN_DEVICE_GPU_KERNEL_CUDA_IMPL_CUDA_OPS_CUDA_DEVICE_INFO_H_

#include <cudnn.h>
#include <cublas_v2.h>
#include <algorithm>
#include <cusolverDn.h>
#include <memory>

#define CUDA_LIB_EXPORT __attribute__((visibility("default")))
#define CUDA_KERNEL_ASSERT(cond)                                                       \
  if (!(cond)) {                                                                       \
    __assert_fail(#cond, __FILE__, static_cast<unsigned int>(__LINE__), __FUNCTION__); \
  }
namespace mindspore {
namespace device {
namespace gpu {
class GPUdeviceInfo {
 public:
  explicit GPUdeviceInfo(const uint32_t device_id);
  ~GPUdeviceInfo() = default;
  inline int threads_num() const { return threads_per_block_; }
  inline int threads_num(int size) const { return std::min(size, threads_per_block_); }
  inline int major_sm() const { return major_sm_; }
  inline float cuda_cap() const { return static_cast<float>(major_sm_ * 10 + minor_sm_) / 10.0; }
  inline int blocks_num(const int total_threads) const {
    return std::min(((total_threads - 1) / threads_per_block_) + 1, max_blocks_);
  }
  size_t share_memory_size() const { return max_share_memory_; }
  void set_check_sm(const bool &flag) { check_sm_ = flag; }
  bool check_sm() const { return check_sm_; }

  static std::shared_ptr<GPUdeviceInfo> GetInstance(uint32_t device_id);

 private:
  GPUdeviceInfo(const GPUdeviceInfo &) = delete;
  GPUdeviceInfo &operator=(const GPUdeviceInfo &) = delete;

  int max_blocks_;
  int threads_per_block_;
  int major_sm_;
  int minor_sm_;
  size_t max_share_memory_;
  bool check_sm_{true};
};

#define CUDA_BLOCKS(device_id, total_threads) \
  mindspore::device::gpu::GPUdeviceInfo::GetInstance(device_id)->blocks_num(total_threads)
#define CUDA_THREADS(device_id) mindspore::device::gpu::GPUdeviceInfo::GetInstance(device_id)->threads_num()
#define CUDA_THREADS_MAXSIZE(device_id, size) \
  mindspore::device::gpu::GPUdeviceInfo::GetInstance(device_id)->threads_num(size)
#define CUDA_MAJOR_SM(device_id) mindspore::device::gpu::GPUdeviceInfo::GetInstance(device_id)->major_sm()
#define CUDA_CAP(device_id) mindspore::device::gpu::GPUdeviceInfo::GetInstance(device_id)->cuda_cap()
#define CUDA_SHARED_MEM_PER_BLOCK(device_id) \
  mindspore::device::gpu::GPUdeviceInfo::GetInstance(device_id)->share_memory_size()

#define MINIUM_SM 6
#define RECOMMEND_SM 7
#define SUPPORTED_CAP 5.3
}  // namespace gpu
}  // namespace device
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PLUGIN_DEVICE_GPU_KERNEL_CUDA_IMPL_CUDA_OPS_CUDA_DEVICE_INFO_H_
