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

import pytest
import numpy as np
import mindspore.nn as nn
import mindspore.ops as ops
import mindspore.context as context
from mindspore.common import dtype as mstype
from mindspore.common import Tensor, Parameter

func_map = {
    "add": ops.TensorScatterAdd,
    "sub": ops.TensorScatterSub,
    "div": ops.TensorScatterDiv,
    "max": ops.TensorScatterMax,
    "min": ops.TensorScatterMin,
}

np_func_map = {
    "mul": lambda a, b: a * b,
    "div": lambda a, b: a / b,
    "add": lambda a, b: a + b,
    "sub": lambda a, b: a - b,
    "max": np.maximum,
    "min": np.minimum,
}


class TestTensorScatterArithmeticNet(nn.Cell):
    def __init__(self, func, input_x, indices, updates):
        super(TestTensorScatterArithmeticNet, self).__init__()
        self.scatter_func = func_map.get(func)()
        self.input_x = Parameter(input_x, name="input_x")
        self.indices = Parameter(indices, name="indices")
        self.updates = Parameter(updates, name="updates")

    def construct(self):
        output = self.scatter_func(self.input_x, self.indices, self.updates)
        return output


def tensor_scatter_np(func, input_x, indices, updates):
    result = input_x.asnumpy().copy()
    indices_np = indices.asnumpy().copy()
    updates_np = updates.asnumpy().copy()

    f = np_func_map.get(func)

    for idx, _ in np.ndenumerate(np.zeros(indices.shape[:-1])):
        upd_idx = tuple(idx)
        out_idx = tuple(indices_np[upd_idx])
        result[out_idx] = f(result[out_idx], updates_np[upd_idx])

    return result


def compare_with_numpy(func, input_x, indices, updates):
    context.set_context(mode=context.GRAPH_MODE, device_target="CPU")
    graph_output = TestTensorScatterArithmeticNet(func, input_x, indices, updates)()
    expected = tensor_scatter_np(func, input_x, indices, updates)
    np.testing.assert_array_almost_equal(graph_output.asnumpy(), expected)

    context.set_context(mode=context.PYNATIVE_MODE, device_target="CPU")
    pynative_output = TestTensorScatterArithmeticNet(func, input_x, indices, updates)()
    np.testing.assert_array_almost_equal(pynative_output.asnumpy(), expected)


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('func', ['add', 'sub', 'div', 'max', 'min'])
@pytest.mark.parametrize('data_type', [mstype.float32, mstype.float64])
@pytest.mark.parametrize('index_type', [mstype.int32])
def test_tensor_scatter_arithmetic_small_float(func, data_type, index_type):
    """
    Feature: TensorScatter* operators.
    Description: test cases for TensorScatter* operator
    Expectation: the result match numpy implementation.
    """
    input_x = Tensor(np.array([[-0.1, 0.3, 3.6], [0.4, 0.5, -3.2]]), data_type)
    indices = Tensor(np.array([[0, 0], [1, 1]]), index_type)
    updates = Tensor(np.array([1.0, 2.2]), data_type)

    compare_with_numpy(func, input_x, indices, updates)


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('func', ['add', 'sub', 'div', 'max', 'min'])
@pytest.mark.parametrize('data_type', [mstype.int8, mstype.int16, mstype.int32, mstype.int64])
@pytest.mark.parametrize('index_type', [mstype.int32])
def test_tensor_scatter_arithmetic_small_int(func, data_type, index_type):
    """
    Feature: TensorScatter* operators.
    Description: test cases for TensorScatter* operator
    Expectation: the result match numpy implementation.
    """
    input_x = Tensor(np.array([1, 2, 3, 4, 5, 6, 7, 8]), data_type)
    indices = Tensor(np.array([[4], [3], [1], [7]]), index_type)
    updates = Tensor(np.array([9, 10, 11, 12]), data_type)

    compare_with_numpy(func, input_x, indices, updates)


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('func', ['add', 'sub', 'div', 'max', 'min'])
@pytest.mark.parametrize('data_type', [mstype.int8, mstype.int16, mstype.int32, mstype.int64])
@pytest.mark.parametrize('index_type', [mstype.int32])
def test_tensor_scatter_arithmetic_multi_dims(func, data_type, index_type):
    """
    Feature: TensorScatter* operators.
    Description: test cases for TensorScatter* operator
    Expectation: the result match numpy implementation.
    """
    input_x = Tensor(np.ones((4, 4, 4)), data_type)
    indices = Tensor(np.array([[0], [2]]), index_type)
    updates = Tensor(
        np.array(
            [
                [[5, 5, 5, 5], [6, 6, 6, 6], [7, 7, 7, 7], [8, 8, 8, 8]],
                [[5, 5, 5, 5], [6, 6, 6, 6], [7, 7, 7, 7], [8, 8, 8, 8]],
            ]
        ),
        data_type,
    )

    compare_with_numpy(func, input_x, indices, updates)


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('func', ['add', 'sub', 'div', 'max', 'min'])
@pytest.mark.parametrize('data_type', [mstype.int8, mstype.int16, mstype.int32, mstype.int64])
@pytest.mark.parametrize('index_type', [mstype.int32])
def test_tensor_scatter_arithmetic_one_value(func, data_type, index_type):
    """
    Feature: TensorScatter* operators.
    Description: test cases for TensorScatter* operator
    Expectation: the result match numpy implementation.
    """
    input_x = Tensor(np.array([[-0.1, 0.3, 3.6], [0.4, 0.5, -3.2]]), data_type)
    indices = Tensor(np.array([[0, 1]]), index_type)
    updates = Tensor(np.array([1.0]), data_type)

    compare_with_numpy(func, input_x, indices, updates)


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('func', ['add', 'sub', 'div', 'max', 'min'])
@pytest.mark.parametrize('data_type', [mstype.int8])
@pytest.mark.parametrize('index_type', [mstype.int32])
def test_tensor_scatter_arithmetic_dim_check(func, data_type, index_type):
    """
    Feature: TensorScatter* operators.
    Description: test cases for invalid input.
    Expectation: raise ValueError.
    """
    input_x = Tensor(np.array([[-0.1, 0.3, 3.6], [0.4, 0.5, -3.2]]), data_type)
    indices = Tensor(np.array([[0, 1, 2]]), index_type)
    updates = Tensor(np.array([1.0]), data_type)

    with pytest.raises(ValueError):
        compare_with_numpy(func, input_x, indices, updates)


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('func', ['add', 'sub', 'div', 'max', 'min'])
@pytest.mark.parametrize('data_type', [mstype.int8, mstype.int16, mstype.int32, mstype.int64])
@pytest.mark.parametrize('index_type', [mstype.int8, mstype.int16])
def test_tensor_scatter_arithmetic_type_check(func, data_type, index_type):
    """
    Feature: TensorScatter* operators.
    Description: test cases for invalid input.
    Expectation: raise TypeError.
    """
    input_x = Tensor(np.array([[-0.1, 0.3, 3.6], [0.4, 0.5, -3.2]]), data_type)
    indices = Tensor(np.array([[0, 1]]), index_type)
    updates = Tensor(np.array([1.0]), data_type)

    with pytest.raises(TypeError):
        compare_with_numpy(func, input_x, indices, updates)
