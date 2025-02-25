/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#include "include/api/graph.h"
#include "include/api/cell.h"
#include "include/api/net.h"
#include "src/cxx_api/graph/graph_data.h"
#include "src/cxx_api/graph/net_data.h"

namespace mindspore {
Graph::Graph() : graph_data_(nullptr) {}

Graph::Graph(const std::shared_ptr<GraphData> &graph_data) : graph_data_(graph_data) {}

Graph::Graph(std::shared_ptr<GraphData> &&graph_data) : graph_data_(graph_data) {}

Graph::Graph(Graph::Type type) : graph_type_(type) {}

Graph::~Graph() {}

Graph::Graph(Net *net) : graph_type_(kExpressionGraph) {
  auto shared = std::make_shared<NetData>(net->shared_from_this());
  net_data_ = shared;
}

Graph::Graph(std::nullptr_t) : graph_data_(nullptr) {}

bool Graph::operator==(std::nullptr_t) const { return graph_data_ == nullptr; }

bool Graph::operator!=(std::nullptr_t) const { return graph_data_ != nullptr; }

ModelType Graph::ModelType() const { return kMindIR; }
}  // namespace mindspore
