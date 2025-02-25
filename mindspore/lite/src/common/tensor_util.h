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

#ifndef MINDSPORE_LITE_SRC_COMMON_TENSOR_UTIL_H_
#define MINDSPORE_LITE_SRC_COMMON_TENSOR_UTIL_H_
#include <vector>
#include <unordered_map>
#include <memory>
#include "src/tensor.h"
#include "nnacl/tensor_c.h"
#include "nnacl/tensor_c_utils.h"
#include "src/tensorlist.h"
#include "nnacl/infer/common_infer.h"
#include "nnacl/tensorlist_c_utils.h"
#include "src/cxx_api/tensor/tensor_impl.h"

namespace mindspore {
namespace lite {
int OutputTensor2TensorC(const std::vector<lite::Tensor *> &tensors_in, std::vector<TensorC *> *tensors_out,
                         std::shared_ptr<Allocator> allocator = nullptr);
void FreeAllTensorC(std::vector<TensorC *> *tensors_in, std::shared_ptr<Allocator> allocator = nullptr);
int Tensor2TensorC(const Tensor *src, TensorC *dst);
int TensorC2Tensor(const TensorC *src, Tensor *dst, std::shared_ptr<Allocator> allocator = nullptr);
void FreeTensorListC(TensorListC *tensorListC, std::shared_ptr<Allocator> allocator = nullptr);
int TensorList2TensorListC(TensorList *src, TensorListC *dst, std::shared_ptr<Allocator> allocator = nullptr);
int TensorListC2TensorList(const TensorListC *src, TensorList *dst);
int GenerateInTensorC(const OpParameter *const parameter, const std::vector<lite::Tensor *> &inputs,
                      std::vector<TensorC *> *in_tensor_c, std::shared_ptr<Allocator> allocator = nullptr);
int GenerateOutTensorC(const OpParameter *const parameter, const std::vector<lite::Tensor *> &outputs,
                       std::vector<TensorC *> *out_tensor_c, std::shared_ptr<Allocator> allocator = nullptr);
int CheckTensorsInvalid(const std::vector<Tensor *> &tensors);
int CheckGraphInputShapes(const std::vector<Tensor *> &inputs,
                          const std::unordered_map<Tensor *, std::vector<int>> &input_shape_map);
std::vector<mindspore::MSTensor> LiteTensorsToMSTensors(const std::vector<lite::Tensor *> &lite_tensors);
void MoveCommonTensorData(Tensor *dst_tensor, Tensor *src_tensor);
void MoveTensorData(Tensor *dst_tensor, Tensor *src_tensor);
void SetTensorData(Tensor *dst_tensor, Tensor *src_tensor);
void SetCommonTensorData(Tensor *dst_tensor, Tensor *src_tensor);
void MoveTensorListTensorData(TensorList *dst_tensor, TensorList *src_tensor);
void SetTensorListTensorData(TensorList *dst_tensor, TensorList *src_tensor);
void SetTensorShape(Tensor *dst, Tensor *src);
void SetTensorListShape(Tensor *dst, Tensor *src);
bool NeedCastTensorListData(Tensor *dst_tensor, Tensor *src_tensor);
bool NeedCastData(Tensor *dst_tensor, Tensor *src_tensor);
int CastTensorData(Tensor *dst_tensor, Tensor *src_tensor, bool support_fp16);
int CastCommonTensorData(Tensor *dst_tensor, Tensor *src_tensor, bool support_fp16);
int CastTensorListTensorData(TensorList *dst_tensor, TensorList *src_tensor, bool support_fp16);
TypeId TensorListDataType(Tensor *tensor);
TensorList *MallocTensorListDataAccordingToTensorListC(Tensor *tensor, TensorListC *tensor_list_c);
int DecodeTensorLsit(Tensor *tensor, const int *src_data);
Tensor *CreateTensorList(const std::vector<int> &shape, const Category &category, const void *src_data);
int CopyTensorListTensorDataType(TensorList *dst_tensor, TensorList *src_tensor);
void SetTensorListTensorDataType(const TypeId &data_type, Tensor *tensor);
}  // namespace lite
}  // namespace mindspore

#endif  // MINDSPORE_LITE_SRC_COMMON_TENSOR_UTIL_H_
