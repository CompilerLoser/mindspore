# Copyright 2022 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

import numpy as np
import pytest
import mindspore.nn as nn
from mindspore import Tensor, context
from mindspore.ops import operations as P


class LpNormNet(nn.Cell):
    def __init__(self, axis, p=2, keep_dims=False, epsilon=1e-12):
        super(LpNormNet, self).__init__()
        self.lp_norm = P.LpNorm(axis, p, keep_dims, epsilon)

    def construct(self, input_x):
        output = self.lp_norm(input_x)
        return output


def lp_norm_np_bencmark(data_type):
    """
    Feature: generate a LpNorm numpy benchmark.
    Description: The input shape need to match input shape.
    Expectation: match to np mindspore LpNorm.
    """
    result = np.array([9.165152, 10.954452]).astype(data_type)
    return result


@pytest.mark.level0
@pytest.mark.env_onecard
@pytest.mark.platform_x86_cpu
@pytest.mark.parametrize("data_type", [np.float32, np.float16])
def test_lp_norm_op(data_type):
    """
    Feature: Test LpNorm.
    Description: The input shape need match to output shape.
    Expectation: match to np benchmark.
    """
    context.set_context(mode=context.GRAPH_MODE)
    input_x = np.array([[[1.0, 2.0], [3.0, 4.0]], [[5.0, 6.0], [7.0, 8.0]]]).astype(data_type)
    error = 1e-6
    if data_type == np.float16:
        error = 1e-3
    benchmark_output = lp_norm_np_bencmark(data_type)
    axis = [0, 1]
    p = 2
    keep_dims = False
    lp_norm = LpNormNet(axis, p, keep_dims)
    output = lp_norm(Tensor(input_x))
    np.testing.assert_allclose(output.asnumpy(), benchmark_output, rtol=error)
    context.set_context(mode=context.PYNATIVE_MODE)
    output = lp_norm(Tensor(input_x))
    np.testing.assert_allclose(output.asnumpy(), benchmark_output, rtol=error)
