mindspore.nn.Vjp
=================

.. py:class:: mindspore.nn.Vjp(fn)

    计算给定网络的向量雅可比积(vector-Jacobian product, VJP)。VJP对应 `反向模式自动微分 <https://www.mindspore.cn/docs/zh-CN/master/design/gradient.html#反向自动微分>`_。

    **参数：**

    - **fn** (Cell) - 基于Cell的网络，用于接收Tensor输入并返回Tensor或者Tensor元组。

    **输入：**

    - **inputs** (Tensor) - 输入网络的入参，单个或多个Tensor。
    - **v** (Tensor or Tuple of Tensor) - 与雅可比矩阵点乘的向量，Shape与网络的输出一致。

    **输出：**

    2个Tensor或Tensor元组构成的元组。

    - **net_output** (Tensor or Tuple of Tensor) - 输入网络的正向计算结果。
    - **vjp** (Tensor or Tuple of Tensor) - 向量雅可比积的结果。
