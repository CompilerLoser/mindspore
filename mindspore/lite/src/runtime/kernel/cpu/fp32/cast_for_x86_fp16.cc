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
#include "src/runtime/kernel/cpu/fp32/cast_fp32.h"
#include "schema/model_generated.h"
#include "src/kernel_registry.h"

using mindspore::kernel::KERNEL_ARCH;
using mindspore::lite::KernelRegistrar;
using mindspore::schema::PrimitiveType_Cast;

namespace mindspore::kernel {
REG_KERNEL(kCPU, kNumberTypeFloat16, PrimitiveType_Cast, LiteKernelCreator<CastCPUKernel>)
}  // namespace mindspore::kernel
