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

#ifndef MINDSPORE_LITE_SRC_CONTROLFLOW_CONTROL_SUBGRAPH_CREATOR_H_
#define MINDSPORE_LITE_SRC_CONTROLFLOW_CONTROL_SUBGRAPH_CREATOR_H_
#include "src/lite_kernel.h"
#include "src/sub_graph_kernel.h"

namespace mindspore::lite {
kernel::SubGraphKernel *CreateControlSubgraph(const kernel::SubGraphType &type, kernel::LiteKernel *kernel);
}  // namespace mindspore::lite
#endif  // MINDSPORE_LITE_SRC_CONTROLFLOW_CONTROL_SUBGRAPH_CREATOR_H_
