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
#include "include/api/context.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace mindspore::lite {
namespace py = pybind11;

void ContextPyBind(const py::module &m) {
  py::enum_<DeviceType>(m, "DeviceType", py::arithmetic())
    .value("kCPU", DeviceType::kCPU)
    .value("kGPU", DeviceType::kGPU)
    .value("kKirinNPU", DeviceType::kKirinNPU)
    .value("kAscend", DeviceType::kAscend);

  py::class_<DeviceInfoContext, std::shared_ptr<DeviceInfoContext>>(m, "DeviceInfoContextBind");

  py::class_<CPUDeviceInfo, DeviceInfoContext, std::shared_ptr<CPUDeviceInfo>>(m, "CPUDeviceInfoBind")
    .def(py::init<>([](bool enable_fp16) {
      auto device_info = std::make_shared<CPUDeviceInfo>();
      device_info->SetEnableFP16(enable_fp16);
      return device_info;
    }))
    .def("get_device_type", &CPUDeviceInfo::GetDeviceType)
    .def("get_enable_fp16", &CPUDeviceInfo::GetEnableFP16);

  py::class_<GPUDeviceInfo, DeviceInfoContext, std::shared_ptr<GPUDeviceInfo>>(m, "GPUDeviceInfoBind")
    .def(py::init<>([](uint32_t device_id, bool enable_fp16, const std::string &precision_mode) {
      auto device_info = std::make_shared<GPUDeviceInfo>();
      device_info->SetDeviceID(device_id);
      device_info->SetEnableFP16(enable_fp16);
      if (precision_mode != "") {
        device_info->SetPrecisionMode(precision_mode);
      }
      return device_info;
    }))
    .def("get_device_type", &GPUDeviceInfo::GetDeviceType)
    .def("get_enable_fp16", &GPUDeviceInfo::GetEnableFP16)
    .def("get_precision_mode", &GPUDeviceInfo::GetPrecisionMode)
    .def("get_device_id", &GPUDeviceInfo::GetDeviceID)
    .def("get_rank_id", &GPUDeviceInfo::GetRankID)
    .def("get_group_size", &GPUDeviceInfo::GetGroupSize);

  py::class_<AscendDeviceInfo, DeviceInfoContext, std::shared_ptr<AscendDeviceInfo>>(m, "AscendDeviceInfoBind")
    .def(py::init<>())
    .def("set_device_id", &AscendDeviceInfo::SetDeviceID)
    .def("get_device_id", &AscendDeviceInfo::GetDeviceID)
    .def("set_input_format",
         [](AscendDeviceInfo &device_info, const std::string &format) { device_info.SetInputFormat(format); })
    .def("get_input_format", &AscendDeviceInfo::GetInputFormat)
    .def("set_input_shape", &AscendDeviceInfo::SetInputShapeMap)
    .def("get_input_shape", &AscendDeviceInfo::GetInputShapeMap)
    .def("set_precision_mode", [](AscendDeviceInfo &device_info,
                                  const std::string &precision_mode) { device_info.SetPrecisionMode(precision_mode); })
    .def("get_precision_mode", &AscendDeviceInfo::GetPrecisionMode)
    .def("set_op_select_impl_mode",
         [](AscendDeviceInfo &device_info, const std::string &op_select_impl_mode) {
           device_info.SetOpSelectImplMode(op_select_impl_mode);
         })
    .def("get_op_select_impl_mode", &AscendDeviceInfo::GetOpSelectImplMode)
    .def("set_dynamic_batch_size", &AscendDeviceInfo::SetDynamicBatchSize)
    .def("get_dynamic_batch_size", &AscendDeviceInfo::GetDynamicBatchSize)
    .def("set_dynamic_image_size",
         [](AscendDeviceInfo &device_info, const std::string &dynamic_image_size) {
           device_info.SetDynamicImageSize(dynamic_image_size);
         })
    .def("get_dynamic_image_size", &AscendDeviceInfo::GetDynamicImageSize)
    .def("set_fusion_switch_config_path",
         [](AscendDeviceInfo &device_info, const std::string &cfg_path) {
           device_info.SetFusionSwitchConfigPath(cfg_path);
         })
    .def("get_fusion_switch_config_path", &AscendDeviceInfo::GetFusionSwitchConfigPath)
    .def("set_insert_op_cfg_path", [](AscendDeviceInfo &device_info,
                                      const std::string &cfg_path) { device_info.SetInsertOpConfigPath(cfg_path); })
    .def("get_insert_op_cfg_path", &AscendDeviceInfo::GetInsertOpConfigPath);

  py::class_<Context, std::shared_ptr<Context>>(m, "ContextBind")
    .def(py::init<>([](int32_t thread_num, int32_t thread_affinity_mode,
                       const std::vector<int> &thread_affinity_core_list, bool enable_parallel) {
      auto context = std::make_shared<Context>();
      context->SetThreadNum(thread_num);
      context->SetThreadAffinity(thread_affinity_mode);
      context->SetThreadAffinity(thread_affinity_core_list);
      context->SetEnableParallel(enable_parallel);
      return context;
    }))
    .def("append_device_info",
         [](Context &context, const std::shared_ptr<DeviceInfoContext> &device_info) {
           context.MutableDeviceInfo().push_back(device_info);
         })
    .def("get_thread_num", &Context::GetThreadNum)
    .def("get_thread_affinity_mode", &Context::GetThreadAffinityMode)
    .def("get_thread_affinity_core_list", &Context::GetThreadAffinityCoreList)
    .def("get_enable_parallel", &Context::GetEnableParallel)
    .def("get_device_list", [](Context &context) {
      std::string result;
      auto &device_list = context.MutableDeviceInfo();
      for (auto &device : device_list) {
        result += std::to_string(device->GetDeviceType());
        result += ", ";
      }
      return result;
    });
}
}  // namespace mindspore::lite
