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

import mindspore.context as context
import mindspore.nn as nn
from mindspore import Tensor
from mindspore.common.parameter import Parameter
from mindspore.ops import operations as P

context.set_context(mode=context.GRAPH_MODE, device_target='CPU')


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('dtype', [np.float16, np.float32])
def test_hshrink(dtype):
    """
    Feature: HShrink cpu kernel
    Description: test the rightness of HShrink cpu kernel
    Expectation: the output[0] is same as numpy
    """
    class NetHShrink(nn.Cell):
        def __init__(self):
            super(NetHShrink, self).__init__()
            self.hard_shrink = P.HShrink(lambd=0.5)
            self.x = Parameter(Tensor(np.array([[0.5, 1, 2.0],
                                                [0.0533, 0.0776, -2.1233]], dtype=dtype)), name='x')

        def construct(self):
            return self.hard_shrink(self.x)

    hshrink = NetHShrink()
    output = hshrink()
    expect = np.array([[0, 1, 2],
                       [0, 0, -2.1233]], dtype=dtype)
    assert np.allclose(output.asnumpy(), expect)
