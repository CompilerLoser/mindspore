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
#ifndef MINDSPORE_LITE_SRC_DELEGATE_TENSORRT_OP_MATMUL_OPT_PLUGIN_H_
#define MINDSPORE_LITE_SRC_DELEGATE_TENSORRT_OP_MATMUL_OPT_PLUGIN_H_
#include <string>
#include <vector>
#include "src/delegate/tensorrt/op/tensorrt_op.h"
#include "src/delegate/tensorrt/op/tensorrt_plugin.h"
#include "src/delegate/tensorrt/cuda_impl/cublas_utils.h"

namespace mindspore::lite {
constexpr char *MATMUL_OPT_PLUGIN_NAME{"MatmulOptPlugin"};
class MatmulOptPlugin : public TensorRTPlugin {
 public:
  MatmulOptPlugin(const std::string name, bool a_trans, bool b_trans)
      : TensorRTPlugin(name, std::string(MATMUL_OPT_PLUGIN_NAME)), a_trans_(a_trans), b_trans_(b_trans) {}

  MatmulOptPlugin(const char *name, const nvinfer1::PluginFieldCollection *fc)
      : TensorRTPlugin(std::string(name), std::string(MATMUL_OPT_PLUGIN_NAME)) {
    const nvinfer1::PluginField *fields = fc->fields;
    a_trans_ = static_cast<const bool *>(fields[0].data)[0];
    b_trans_ = static_cast<const bool *>(fields[1].data)[0];
  }

  MatmulOptPlugin(const char *name, const void *serialData, size_t serialLength)
      : TensorRTPlugin(std::string(name), std::string(MATMUL_OPT_PLUGIN_NAME)) {
    DeserializeValue(&serialData, &serialLength, &a_trans_, sizeof(bool));
    DeserializeValue(&serialData, &serialLength, &b_trans_, sizeof(bool));
  }

  MatmulOptPlugin() = delete;

  // IPluginV2DynamicExt Methods
  nvinfer1::IPluginV2DynamicExt *clone() const noexcept override;
  nvinfer1::DimsExprs getOutputDimensions(int outputIndex, const nvinfer1::DimsExprs *inputs, int nbInputs,
                                          nvinfer1::IExprBuilder &exprBuilder) noexcept override;
  void configurePlugin(const nvinfer1::DynamicPluginTensorDesc *in, int nbInputs,
                       const nvinfer1::DynamicPluginTensorDesc *out, int nbOutputs) noexcept override;
  int enqueue(const nvinfer1::PluginTensorDesc *inputDesc, const nvinfer1::PluginTensorDesc *outputDesc,
              const void *const *inputs, void *const *outputs, void *workspace, cudaStream_t stream) noexcept override;
  int initialize() noexcept override;
  void terminate() noexcept override;
  size_t getSerializationSize() const noexcept override;
  void serialize(void *buffer) const noexcept override;

 private:
  int RunBatchedMatmul(const nvinfer1::PluginTensorDesc *inputDesc, const nvinfer1::PluginTensorDesc *outputDesc,
                       const void *const *inputs, void *const *outputs, void *workspace, cudaStream_t stream);

  bool a_trans_{false};
  bool b_trans_{false};
  cublasHandle_t cublas_handle_{nullptr};
  cublasOperation_t operations_[2]{CUBLAS_OP_N, CUBLAS_OP_N};
  cudaDataType data_types_[4]{CUDA_R_32F, CUDA_R_32F, CUDA_R_32F, CUDA_R_32F};
  std::vector<const void *> a_addrs_;
  std::vector<const void *> b_addrs_;
  std::vector<void *> c_addrs_;
  void **a_device_addrs_{nullptr};
  void **b_device_addrs_{nullptr};
  void **c_device_addrs_{nullptr};
};
class MatmulOptPluginCreater : public TensorRTPluginCreater<MatmulOptPlugin> {
 public:
  MatmulOptPluginCreater() : TensorRTPluginCreater(std::string(MATMUL_OPT_PLUGIN_NAME)) {}
};
}  // namespace mindspore::lite
#endif  // MINDSPORE_LITE_SRC_DELEGATE_TENSORRT_OP_MATMUL_OPT_PLUGIN_H_
