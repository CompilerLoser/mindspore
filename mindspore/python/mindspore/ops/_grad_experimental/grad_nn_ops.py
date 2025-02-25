# Copyright 2021 Huawei Technologies Co., Ltd
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


"""Define the grad rules of neural network related operations."""
from mindspore import Tensor
from mindspore.ops.primitive import constexpr
from mindspore.common import dtype as mstype
from .._grad.grad_base import bprop_getters
from .. import operations as P
from ..composite.multitype_ops.zeros_like_impl import zeros_like
from ..operations import _grad_ops as G
from ..operations.nn_ops import FractionalMaxPool
from ..operations._grad_ops import FractionalMaxPoolGrad
from ..operations.nn_ops import FractionalMaxPool3DWithFixedKsize
from ..operations._grad_ops import FractionalMaxPool3DGradWithFixedKsize
from ..operations.nn_ops import FractionalAvgPool
from ..operations._grad_ops import FractionalAvgPoolGrad
from ..operations.nn_ops import NthElement


@bprop_getters.register(P.CTCLossV2)
def get_bprop_ctc_loss_v2(self):
    """Grad definition for `CTCLossV2` operation"""
    ctc_loss_grad = P.CTCLossV2Grad(self.blank, self.reduction, self.zero_infinity)

    def bprop(log_probs, targets, input_lengths, target_lengths, out, dout):
        grad = ctc_loss_grad(dout[0], log_probs, targets, input_lengths, target_lengths, out[0], out[1])
        return grad, zeros_like(targets), zeros_like(input_lengths), zeros_like(target_lengths)

    return bprop


@bprop_getters.register(P.SoftMarginLoss)
def get_bprop_soft_margin_loss(self):
    """Grad definition for `SoftMarginLoss` operation."""
    grad = G.SoftMarginLossGrad(reduction=self.reduction)

    def bprop(predict, label, out, dout):
        dx = grad(predict, label, dout)
        dy = grad(label, predict, dout)
        return dx, dy

    return bprop


@bprop_getters.register(P.SoftShrink)
def get_bprop_softshrink(self):
    """Grad definition for `SoftShrink` operation."""
    input_grad = G.SoftShrinkGrad(self.lambd)

    def bprop(input_x, out, dout):
        dx = input_grad(dout, input_x)
        return (dx,)

    return bprop


@bprop_getters.register(P.HShrink)
def get_bprop_hshrink(self):
    """Grad definition for `HShrinkGrad` operation."""
    grad = G.HShrinkGrad(self.lambd)

    def bprop(features, out, gradients):
        dx = grad(gradients, features)
        return (dx,)

    return bprop


@bprop_getters.register(P.CeLU)
def get_bprop_celu(self):
    """Grad definition for `CeLU` operation."""
    alpha = self.alpha
    greater_equal = P.GreaterEqual()
    less = P.Less()

    def bprop(x, out, dout):
        greater = greater_equal(x, 0.0)
        lesser = less(x, 0.0)
        dx = dout * (greater * 1.0 + lesser * (out / alpha + 1.0))
        return (dx,)

    return bprop


@bprop_getters.register(P.GridSampler3D)
def get_bprop_grid_sampler_3d(self):
    """Grad definition for `GridSampler3D` operation."""
    grad = G.GridSampler3DGrad(self.interpolation_mode, self.padding_mode, self.align_corners)

    def bprop(input_x, grid, out, dout):
        dx, dgrid = grad(dout, input_x, grid)
        return dx, dgrid
    return bprop


@bprop_getters.register(NthElement)
def get_bprop_nth_element(self):
    """Grad definition for `NthElement` operation."""
    expand_dims = P.ExpandDims()
    cast = P.Cast()
    equal = P.Equal()
    reduce_sum = P.ReduceSum()
    divide = P.Div()
    def bprop(input_x, n, out, dout):
        indicators = cast(equal(expand_dims(out, -1), input_x), input_x.dtype)
        dout = expand_dims(dout, -1)
        num_select = expand_dims(reduce_sum(indicators, -1), -1)
        return divide(indicators, num_select) * dout, None

    return bprop


@bprop_getters.register(FractionalMaxPool)
def get_bprop_fractional_max_pool(self):
    """Grad definition for `FractionalMaxPool` operation."""
    fractional_max_pool_grad = FractionalMaxPoolGrad(self.overlapping)

    def bprop(x, out, dout):
        dx = fractional_max_pool_grad(x, out[0], dout[0], out[1], out[2])
        return (dx,)

    return bprop


@bprop_getters.register(FractionalMaxPool3DWithFixedKsize)
def get_bprop_fractional_max_pool3d_with_fixed_ksize(self):
    """Grad definition for `FractionalMaxPool3DWithFixedKsize` operation."""
    fractional_max_pool3d_grad_with_fixed_ksize = FractionalMaxPool3DGradWithFixedKsize(data_format=self.data_format)

    def bprop(x, random_samples, out, dout):
        dx = fractional_max_pool3d_grad_with_fixed_ksize(x, dout[0], out[1])
        return (dx, zeros_like(random_samples))

    return bprop


@constexpr
def _create_tensor(x_shape):
    return Tensor(x_shape, mstype.int64)


@bprop_getters.register(FractionalAvgPool)
def get_bprop_fractional_avg_pool(self):
    """Grad definition for `FractionalAvgPool` operation."""
    fractional_avg_pool_grad = FractionalAvgPoolGrad(overlapping=self.overlapping)

    def bprop(x, out, dout):
        dx = fractional_avg_pool_grad(_create_tensor(x.shape), dout[0], out[1], out[2])
        return (dx,)

    return bprop
