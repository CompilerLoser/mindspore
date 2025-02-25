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

#ifndef MINDSPORE_CCSRC_BACKEND_OPTIMIZER_GRAPH_KERNEL_CLEAN_INSERTER_H_
#define MINDSPORE_CCSRC_BACKEND_OPTIMIZER_GRAPH_KERNEL_CLEAN_INSERTER_H_

#include <utility>
#include <vector>
#include <string>
#include "backend/common/optimizer/optimizer.h"

namespace mindspore::graphkernel {
struct CleanZeroUserInfo {
  CNodePtr op_node{nullptr};
  size_t real_output_index{0};
  size_t real_output_num{0};
};

class CleanInserter : public opt::Pass {
 public:
  explicit CleanInserter(const std::string &name = "clean_inserter") : Pass(name) {}
  ~CleanInserter() override = default;

 protected:
  virtual void CorrectKernelBuildInfo(const AnfNodePtr &composite_node,
                                      const std::vector<std::pair<CleanZeroUserInfo, AnfNodePtr>> &clean_infos);
  virtual CNodePtr CreateCleanCompositeNode(const CleanZeroUserInfo &op_info, const FuncGraphPtr &main_graph,
                                            TypeId dst_type);
  CNodePtr InsertUpdateState(const FuncGraphPtr &main_graph, const AnfNodePtr &node) const;
  void CreateAssignNodeAndCorrectReturn(const FuncGraphPtr &sub_graph,
                                        const std::vector<std::pair<CleanZeroUserInfo, AnfNodePtr>> &parameters_infos);
  virtual void ProcessOriginCNode(
    const AnfNodePtr &composite_node,
    const std::vector<std::pair<CleanZeroUserInfo, AnfNodePtr>> &info_and_broadcast_to_nodes,
    bool atomic_add_attr = true);
};
}  // namespace mindspore::graphkernel
#endif  // MINDSPORE_CCSRC_BACKEND_OPTIMIZER_GRAPH_KERNEL_CLEAN_INSERTER_H_
