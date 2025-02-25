/**
 * This is the C++ adaptation and derivative work of Myia (https://github.com/mila-iqia/myia/).
 *
 * Copyright 2019-2022 Huawei Technologies Co., Ltd
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

#include "frontend/optimizer/clean.h"
#include <iterator>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <utility>
#include "abstract/abstract_value.h"
#include "base/base.h"
#include "mindspore/core/ops/core_ops.h"
#include "pipeline/jit/debug/trace.h"
#include "frontend/optimizer/opt.h"
#include "frontend/operator/composite/composite.h"
#include "ir/anf.h"
#include "ir/value.h"
#include "pipeline/jit/parse/resolve.h"
#include "utils/hash_map.h"

namespace mindspore {
/* namespace to support opt */
namespace opt {
using mindspore::abstract::AbstractAttribute;
using mindspore::abstract::AbstractBase;
using mindspore::abstract::AbstractBasePtr;
using mindspore::abstract::AbstractCOOTensor;
using mindspore::abstract::AbstractDictionary;
using mindspore::abstract::AbstractDictionaryPtr;
using mindspore::abstract::AbstractList;
using mindspore::abstract::AbstractListPtr;
using mindspore::abstract::AbstractRowTensor;
using mindspore::abstract::AbstractScalar;
using mindspore::abstract::AbstractSequence;
using mindspore::abstract::AbstractSequencePtr;
using mindspore::abstract::AbstractTuple;
using mindspore::abstract::AbstractTuplePtr;

namespace {
static constexpr size_t kMaxSeqRecursiveDepth = 6;
void CheckInputsSize(const CNodePtr &cnode, size_t expect_size) {
  if (cnode->size() != expect_size) {
    std::string op_name = GetCNodeFuncName(cnode);
    MS_LOG(EXCEPTION) << op_name << " should have " << expect_size << " inputs, but got " << cnode->size();
  }
}

template <typename T>
std::shared_ptr<T> GetAbstract(const AnfNodePtr &node) {
  return dyn_cast<T>(node->abstract());
}

// ===========================================================================
// BaseRewriter provides a common framework for data struct simplify.
// ===========================================================================
class BaseRewriter : protected SimpleRewriter {
 public:
  BaseRewriter(const FuncGraphPtr &root_graph, const FuncGraphManagerPtr &manager)
      : SimpleRewriter(root_graph, manager) {}
  ~BaseRewriter() override = default;

  bool Execute() {
    bool changed = Run();
    if (changed) {
      UpdateAbstracts();
    }
    return changed;
  }

 protected:
  virtual AnfNodePtr ConvertPrimitiveCNode(const CNodePtr &cnode, const PrimitivePtr &prim) = 0;
  virtual AnfNodePtr ConvertValueNode(const ValueNodePtr &value_node, const ValuePtr &value) = 0;
  virtual AbstractBasePtr ConvertAbstract(const AbstractBasePtr &abs) = 0;

  AnfNodePtr NodeRewrite(const AnfNodePtr &node) override {
    auto new_node = ConvertNode(node);
    if (new_node != nullptr) {
      new_node->set_abstract(node->abstract());
    }
    return new_node;
  }

  AnfNodePtr ConvertNode(const AnfNodePtr &node) {
    auto cnode = node->cast<CNodePtr>();
    if (cnode != nullptr) {
      if (cnode->size() == 0) {
        return nullptr;
      }
      // Get primitive from cnode.
      auto prim = GetValueNode<PrimitivePtr>(cnode->input(0));
      if (prim == nullptr) {
        return nullptr;
      }
      // Call primitive cnode converter.
      return ConvertPrimitiveCNode(cnode, prim);
    }
    auto value_node = node->cast<ValueNodePtr>();
    if (value_node != nullptr) {
      const auto &value = value_node->value();
      if (value == nullptr) {
        return nullptr;
      }
      // Call value node converter.
      return ConvertValueNode(value_node, value);
    }
    return nullptr;
  }

  void UpdateAbstracts() {
    const auto &nodes = manager_->all_nodes();
    for (const auto &node : nodes) {
      const auto &abs = node->abstract();
      if (abs == nullptr) {
        continue;
      }
      bool is_interpret_dict = false;
      // Do not convert the abstract of Interpret node(AbstractDictionary) to AbstractSequence.
      if (abs->isa<AbstractDictionary>()) {
        AbstractDictionaryPtr abs_dict = abs->cast<AbstractDictionaryPtr>();
        auto &dict_elements = abs_dict->elements();
        for (auto &element : dict_elements) {
          TypePtr type = element.second->GetTypeTrack();
          MS_EXCEPTION_IF_NULL(type);
          auto value = element.second->BuildValue();
          MS_EXCEPTION_IF_NULL(value);
          if (type->type_id() == kMetaTypeExternal && value->isa<parse::InterpretedObject>()) {
            is_interpret_dict = true;
            break;
          }
        }
      }
      if (is_interpret_dict) {
        continue;
      }
      // Call abstract converter.
      auto new_abs = ConvertAbstract(abs);
      if (new_abs != nullptr) {
        node->set_abstract(new_abs);
      }
    }
  }
};

// ===========================================================================
// SimplifyDataStructuresRewriter convert ObjectClass, Dictionary to Tuple.
// ===========================================================================
class SimplifyDataStructuresRewriter : public BaseRewriter {
 public:
  using ThisClass = SimplifyDataStructuresRewriter;
  SimplifyDataStructuresRewriter(const FuncGraphPtr &root_graph, const FuncGraphManagerPtr &manager)
      : BaseRewriter(root_graph, manager) {}
  ~SimplifyDataStructuresRewriter() override = default;

 protected:
  static std::string GetStringValue(const AnfNodePtr &node) {
    auto str = GetValueNode<StringImmPtr>(node);
    if (str == nullptr) {
      return "";
    }
    return str->value();
  }

  static int64_t GetAttrIndex(const std::vector<AbstractAttribute> &attrs, const std::string &name) {
    auto n_attrs = attrs.size();
    for (size_t i = 0; i < n_attrs; ++i) {
      if (attrs[i].first == name) {
        return SizeToLong(i);
      }
    }
    return SizeToLong(n_attrs);
  }

  static CNodePtr NewTupleGetCNode(const AnfNodePtr &cnode, const AnfNodePtr &data_node,
                                   const std::vector<AbstractAttribute> &attributes, const AnfNodePtr &name_node) {
    int64_t index = GetAttrIndex(attributes, GetStringValue(name_node));
    auto index_node = NewValueNode(index);
    auto prim_node = NewValueNode(prim::kPrimTupleGetItem);
    return cnode->func_graph()->NewCNode({prim_node, data_node, index_node});
  }

  // From:
  //   DictGetItem(data:AbstractDictionary, cons:StringImm)
  // To:
  //   TupleGetItem(data, index:Int64Imm)
  AnfNodePtr ConvertDictGetItemToTupleGetItem(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    MS_EXCEPTION_IF_NULL(node->func_graph());

    // Inputs should be [dict_getitem, dict, item]
    const size_t expect_inputs_size = 3;
    CheckInputsSize(node, expect_inputs_size);

    constexpr size_t data_index = 1;
    constexpr size_t attr_index = 2;
    const auto &inputs = node->inputs();
    auto &data = inputs[data_index];
    auto &attr = inputs[attr_index];
    MS_EXCEPTION_IF_NULL(data);
    MS_EXCEPTION_IF_NULL(attr);

    auto abs_dict = GetAbstract<AbstractDictionary>(data);
    if (abs_dict == nullptr) {
      return nullptr;
    }
    return NewTupleGetCNode(node, data, abs_dict->elements(), attr);
  }

  // From:
  //   DictSetItem(data:AbstractDictionary, cons:StringImm, value)
  // To:
  //   TupleSetItem(data, index:Int64Imm, value)
  // Or:
  //   tuple_add(data, value)
  AnfNodePtr ConvertDictSetItemToTupleSetItem(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    MS_EXCEPTION_IF_NULL(node->func_graph());

    // Inputs should be [dict_setitem, dict, item, value]
    const size_t expect_inputs_size = 4;
    CheckInputsSize(node, expect_inputs_size);

    const size_t data_index = 1;
    const size_t cons_index = 2;
    const size_t item_value_index = 3;
    const auto &inputs = node->inputs();
    auto &data = inputs[data_index];
    auto &cons = inputs[cons_index];
    auto &item_value = inputs[item_value_index];
    MS_EXCEPTION_IF_NULL(data);
    MS_EXCEPTION_IF_NULL(cons);

    auto abs_dict = GetAbstract<AbstractDictionary>(data);
    if (abs_dict == nullptr) {
      return nullptr;
    }
    int64_t index = GetAttrIndex(abs_dict->elements(), GetStringValue(cons));
    if (index >= static_cast<int64_t>(abs_dict->elements().size())) {
      // For dictionary set, if the key does not exist, we should create a new item.
      std::vector<AnfNodePtr> make_tuple_inputs = {NewValueNode(prim::kPrimMakeTuple)};
      for (size_t i = 0; i < abs_dict->elements().size(); ++i) {
        auto tuple_getitem_i =
          node->func_graph()->NewCNode({NewValueNode(prim::kPrimTupleGetItem), data, NewValueNode(SizeToLong(i))});
        (void)make_tuple_inputs.emplace_back(tuple_getitem_i);
      }
      (void)make_tuple_inputs.emplace_back(item_value);
      return node->func_graph()->NewCNode(make_tuple_inputs);
    }
    auto index_node = NewValueNode(index);
    return node->func_graph()->NewCNode({NewValueNode(prim::kPrimTupleSetItem), data, index_node, item_value});
  }

  // From:
  //   MakeDict(name, input)
  // To:
  //   input
  AnfNodePtr EraseMakeDictNode(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    constexpr size_t expect_inputs_size = 3;
    constexpr size_t input_index = 2;
    CheckInputsSize(node, expect_inputs_size);
    return node->input(input_index);
  }

  // From:
  //   DictGetValues(dict:AbstractDictionary)
  // To:
  //   dict
  AnfNodePtr EraseDictGetValues(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    constexpr size_t expect_inputs_size = 2;
    CheckInputsSize(node, expect_inputs_size);
    return node->input(1);
  }

  // From:
  //   DictItems(dict:AbstractDictionary)
  // To:
  //   kPrimMakeList(MakeTuple(key0, TupleGetItem(dict, 0)), ...)
  AnfNodePtr EraseDictItems(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    auto fg = node->func_graph();
    MS_EXCEPTION_IF_NULL(fg);
    constexpr size_t expect_inputs_size = 2;
    CheckInputsSize(node, expect_inputs_size);

    const auto &input = node->input(1);
    auto abs_dict = GetAbstract<AbstractDictionary>(input);
    if (abs_dict == nullptr) {
      return nullptr;
    }
    const auto &elements = abs_dict->elements();
    std::vector<AnfNodePtr> new_inputs;
    new_inputs.reserve(elements.size() + 1);
    (void)new_inputs.emplace_back(NewValueNode(prim::kPrimMakeList));
    for (size_t i = 0; i < elements.size(); ++i) {
      auto index_node = NewValueNode(static_cast<int64_t>(i));
      auto key_node = NewValueNode(elements[i].first);
      auto value_node = fg->NewCNode({NewValueNode(prim::kPrimTupleGetItem), input, index_node});
      auto tuple_node = fg->NewCNode({NewValueNode(prim::kPrimMakeTuple), key_node, value_node});
      (void)new_inputs.emplace_back(tuple_node);
    }
    return fg->NewCNode(std::move(new_inputs));
  }

  // From:
  //   MakeKeywordArg(key, value)
  // To:
  //   value
  AnfNodePtr EraseMakeKeywordArgNode(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    // Inputs should be [make_keyword_arg, key, value]
    constexpr size_t expect_input_size = 3;
    constexpr size_t value_inputs_index = 2;
    CheckInputsSize(node, expect_input_size);
    return node->input(value_inputs_index);
  }

  // From:
  //   ExtractKeywordArg(arg, key)
  // To:
  //   key
  AnfNodePtr EraseExtractKeywordArg(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    // Inputs should be [extract_keyword_arg, arg, key]
    const size_t expect_inputs_size = 3;
    CheckInputsSize(node, expect_inputs_size);
    constexpr size_t key_index = 2;
    return node->input(key_index);
  }

  // dict(k0:v0, k1:v1, ...) --> tuple(v0, v1, ...)
  ValueTuplePtr DictToTuple(const ValueDictionaryPtr &dict) {
    const auto &elements = dict->value();
    std::vector<ValuePtr> values;
    values.reserve(elements.size());
    (void)std::transform(elements.begin(), elements.end(), std::back_inserter(values),
                         [](const auto &element) { return element.second; });
    return std::make_shared<ValueTuple>(values);
  }

  using Converter = AnfNodePtr (ThisClass::*)(const CNodePtr &);
  using ConverterMap = mindspore::HashMap<PrimitivePtr, Converter, PrimitiveHasher, PrimitiveEqual>;
  static inline const ConverterMap converters_{
    {prim::kPrimDictGetItem, &ThisClass::ConvertDictGetItemToTupleGetItem},
    {prim::kPrimDictSetItem, &ThisClass::ConvertDictSetItemToTupleSetItem},
    {prim::kPrimDictGetValues, &ThisClass::EraseDictGetValues},
    {prim::kPrimMakeDict, &ThisClass::EraseMakeDictNode},
    {prim::kPrimMakeKeywordArg, &ThisClass::EraseMakeKeywordArgNode},
    {prim::kPrimExtractKeywordArg, &ThisClass::EraseExtractKeywordArg},
    {prim::kPrimDictItems, &ThisClass::EraseDictItems},
  };

  AnfNodePtr ConvertPrimitiveCNode(const CNodePtr &cnode, const PrimitivePtr &prim) override {
    // Find cnode converter by primitive.
    auto iter = converters_.find(prim);
    if (iter == converters_.end()) {
      return nullptr;
    }
    // Call converter.
    return (this->*(iter->second))(cnode);
  }

  AnfNodePtr ConvertValueNode(const ValueNodePtr &, const ValuePtr &value) override {
    // Convert Dictionary value node.
    if (value->isa<ValueDictionary>()) {
      return NewValueNode(DictToTuple(value->cast<ValueDictionaryPtr>()));
    }
    return nullptr;
  }

  static std::shared_ptr<AbstractTuple> MakeAbstractTuple(const std::vector<AbstractAttribute> &attrs) {
    std::vector<AbstractBasePtr> elements;
    elements.reserve(attrs.size());
    (void)std::transform(attrs.begin(), attrs.end(), std::back_inserter(elements),
                         [](const auto &item) { return item.second; });
    return std::make_shared<AbstractTuple>(std::move(elements));
  }

  // AbstractDictionary --> AbstractSequence.
  static AbstractSequencePtr ConvertToAbstractSequence(const AbstractBasePtr &abs, size_t depth) {
    if (depth > kMaxSeqRecursiveDepth) {
      MS_LOG(EXCEPTION) << "List or Dict nesting is not allowed more than " << kMaxSeqRecursiveDepth << " levels.";
    }
    auto abs_seq = abs->cast<AbstractSequencePtr>();
    if (abs_seq != nullptr) {
      const auto &seq_elements = abs_seq->elements();
      // First we check if elements should be converted,
      // changed_elements maps old element to new element.
      mindspore::HashMap<AbstractBasePtr, AbstractBasePtr> changed_elements;
      for (const auto &element : seq_elements) {
        auto new_element = ConvertToAbstractSequence(element, depth + 1);
        if (new_element != nullptr) {
          (void)changed_elements.emplace(element, new_element);
        }
      }
      if (changed_elements.empty()) {
        // Here the AbstractList don't need to convert to AbstractTuple.
        return nullptr;
      }
      // Always make new AbstractSequence when elements changed.
      std::vector<AbstractBasePtr> elements;
      elements.reserve(seq_elements.size());
      for (const auto &element : seq_elements) {
        auto iter = changed_elements.find(element);
        if (iter != changed_elements.end()) {
          (void)elements.emplace_back(iter->second);
        } else {
          (void)elements.emplace_back(element);
        }
      }
      // Here the AbstractList don't need to convert to AbstractTuple.
      if (abs_seq->isa<AbstractList>()) {
        return std::make_shared<AbstractList>(std::move(elements));
      } else {
        return std::make_shared<AbstractTuple>(std::move(elements));
      }
    }
    // AbstractDictionary --> AbstractTuple.
    auto abs_dict = abs->cast<AbstractDictionaryPtr>();
    if (abs_dict != nullptr) {
      const auto &dict_elements = abs_dict->elements();
      std::vector<AbstractBasePtr> elements;
      elements.reserve(dict_elements.size());
      for (const auto &element : dict_elements) {
        auto new_element = ConvertToAbstractSequence(element.second, depth + 1);
        if (new_element != nullptr) {
          (void)elements.emplace_back(new_element);
        } else {
          (void)elements.emplace_back(element.second);
        }
      }
      return std::make_shared<AbstractTuple>(elements);
    }
    return nullptr;
  }

  AbstractBasePtr ConvertAbstract(const AbstractBasePtr &abs) override {
    // AbstractDictionary --> AbstractSequence.
    return ConvertToAbstractSequence(abs, 0);
  }
};

// ==================================================================
// CleanAfterOptARewriter converts List, Sparse, RowTensor to Tuple.
// ==================================================================
class CleanAfterOptARewriter : public BaseRewriter {
 public:
  using ThisClass = CleanAfterOptARewriter;
  CleanAfterOptARewriter(const FuncGraphPtr &root_graph, const FuncGraphManagerPtr &manager)
      : BaseRewriter(root_graph, manager) {}
  ~CleanAfterOptARewriter() override = default;

 protected:
  // From:
  //   MakeList(arg1, arg2, ...)
  // To:
  //   MakeTuple(arg1, arg2, ...)
  AnfNodePtr ConvertMakeListToMakeTuple(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    MS_EXCEPTION_IF_NULL(node->func_graph());

    std::vector<AnfNodePtr> inputs;
    inputs.reserve(node->size());
    (void)inputs.emplace_back(NewValueNode(prim::kPrimMakeTuple));
    // Inputs of node should be [make_list, item1, item2, ...], so offset by 1 to get items;
    (void)inputs.insert(inputs.end(), node->inputs().begin() + 1, node->inputs().end());
    return node->func_graph()->NewCNode(std::move(inputs));
  }

  // From:
  //   ListGetItem(list, cons)
  // To:
  //   TupleGetItem(list, cons)
  AnfNodePtr ConvertListGetItemToTupleGetItem(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    MS_EXCEPTION_IF_NULL(node->func_graph());

    // Inputs should be [list_getitem, list, item]
    constexpr size_t expect_input_size = 3;
    CheckInputsSize(node, expect_input_size);
    constexpr size_t data_index = 1;
    constexpr size_t cons_index = 2;
    const auto &inputs = node->inputs();
    auto &data = inputs[data_index];
    auto &cons = inputs[cons_index];
    return node->func_graph()->NewCNode({NewValueNode(prim::kPrimTupleGetItem), data, cons});
  }

  // From:
  //   ListSetItem(list, index, item)
  // To:
  //   TupleSetItem(list, index, item)
  AnfNodePtr ConvertListSetItemToTupleSetItem(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    MS_EXCEPTION_IF_NULL(node->func_graph());

    // Inputs should be [list_setitem, list, index, item]
    const size_t expect_inputs_size = 4;
    CheckInputsSize(node, expect_inputs_size);

    const size_t data_index = 1;
    const size_t cons_index = 2;
    const size_t value_index = 3;
    const auto &inputs = node->inputs();
    auto &data = inputs[data_index];
    auto &cons = inputs[cons_index];
    auto &value = inputs[value_index];
    return node->func_graph()->NewCNode({NewValueNode(prim::kPrimTupleSetItem), data, cons, value});
  }

  // From:
  //   MakeSparse(indices, values, dense_shape)
  // To:
  //   MakeTuple(indices, values, dense_shape)
  AnfNodePtr ConvertMakeSparseToMakeTuple(const CNodePtr &node) {
    MS_EXCEPTION_IF_NULL(node);
    MS_EXCEPTION_IF_NULL(node->func_graph());

    std::vector<AnfNodePtr> inputs;
    inputs.reserve(node->size());
    (void)inputs.emplace_back(NewValueNode(prim::kPrimMakeTuple));
    // Inputs of node should be [make_sparse, indices, values, dense_shape], so offset by 1 to get items.
    (void)inputs.insert(inputs.end(), node->inputs().begin() + 1, node->inputs().end());
    return node->func_graph()->NewCNode(std::move(inputs));
  }

  // From:
  //   RowTensorGetXXX(sparse) # index
  // To:
  //   TupleGetItem(sparse, index)
  AnfNodePtr ConvertSparseGetAttr(const CNodePtr &node, const int64_t index) {
    MS_EXCEPTION_IF_NULL(node);
    MS_EXCEPTION_IF_NULL(node->func_graph());

    // Inputs should be [sparse_getattr, sparse]
    constexpr size_t expect_input_index = 2;
    CheckInputsSize(node, expect_input_index);

    const auto &sparse = node->input(1);
    auto cons_node = NewValueNode(index);
    return node->func_graph()->NewCNode({NewValueNode(prim::kPrimTupleGetItem), sparse, cons_node});
  }

  AnfNodePtr ConvertSparseGetIndices(const CNodePtr &node) { return ConvertSparseGetAttr(node, 0); }

  AnfNodePtr ConvertSparseGetValues(const CNodePtr &node) { return ConvertSparseGetAttr(node, 1); }

  AnfNodePtr ConvertSparseGetDenseShape(const CNodePtr &node) {
    constexpr int64_t dense_shape_index = 2;
    return ConvertSparseGetAttr(node, dense_shape_index);
  }

  using Converter = AnfNodePtr (ThisClass::*)(const CNodePtr &);
  using ConverterMap = mindspore::HashMap<PrimitivePtr, Converter, PrimitiveHasher, PrimitiveEqual>;
  static inline const ConverterMap converters_{
    {prim::kPrimMakeList, &ThisClass::ConvertMakeListToMakeTuple},
    {prim::kPrimListGetItem, &ThisClass::ConvertListGetItemToTupleGetItem},
    {prim::kPrimListSetItem, &ThisClass::ConvertListSetItemToTupleSetItem},
    {prim::kPrimMakeRowTensor, &ThisClass::ConvertMakeSparseToMakeTuple},
    {prim::kPrimRowTensorGetIndices, &ThisClass::ConvertSparseGetIndices},
    {prim::kPrimRowTensorGetValues, &ThisClass::ConvertSparseGetValues},
    {prim::kPrimRowTensorGetDenseShape, &ThisClass::ConvertSparseGetDenseShape},
  };

  AnfNodePtr ConvertPrimitiveCNode(const CNodePtr &cnode, const PrimitivePtr &prim) override {
    // Find cnode converter by primitive.
    auto iter = converters_.find(prim);
    if (iter == converters_.end()) {
      return nullptr;
    }
    // Call converter.
    return (this->*(iter->second))(cnode);
  }

  // ValueList --> ValueTuple
  static ValueTuplePtr ConvertValueListToValueTuple(const ValueListPtr &value_list, size_t depth) {
    if (depth > kMaxSeqRecursiveDepth) {
      MS_LOG(EXCEPTION) << "List nesting is not allowed more than " << kMaxSeqRecursiveDepth << " levels.";
    }
    const auto &list_elements = value_list->value();
    std::vector<ValuePtr> elements;
    elements.reserve(list_elements.size());
    for (const auto &element : list_elements) {
      if (element->isa<ValueList>()) {
        (void)elements.emplace_back(ConvertValueListToValueTuple(element->cast<ValueListPtr>(), depth + 1));
      } else {
        (void)elements.emplace_back(element);
      }
    }
    return std::make_shared<ValueTuple>(elements);
  }

  AnfNodePtr ConvertValueNode(const ValueNodePtr &, const ValuePtr &value) override {
    auto value_list = dyn_cast<ValueList>(value);
    if (value_list != nullptr) {
      return std::make_shared<ValueNode>(ConvertValueListToValueTuple(value_list, 0));
    }
    return nullptr;
  }

  // AbstractSequence, AbstractDict, AbstractCOOTensor, AbstractRowTensor --> AbstractTuple.
  static AbstractTuplePtr ConvertToAbstractTuple(const AbstractBasePtr &abs, size_t depth) {
    if (depth > kMaxSeqRecursiveDepth) {
      MS_LOG(EXCEPTION) << "List or Dict nesting is not allowed more than " << kMaxSeqRecursiveDepth << " levels.";
    }
    // AbstractList --> AbstractTuple.
    auto abs_seq = abs->cast<AbstractSequencePtr>();
    if (abs_seq != nullptr) {
      const auto &seq_elements = abs_seq->elements();
      // First we check if elements should be converted,
      // changed_elements maps old element to new element.
      mindspore::HashMap<AbstractBasePtr, AbstractBasePtr> changed_elements;
      for (const auto &element : seq_elements) {
        auto new_element = ConvertToAbstractTuple(element, depth + 1);
        if (new_element != nullptr) {
          (void)changed_elements.emplace(element, new_element);
        }
      }
      if (changed_elements.empty()) {
        if (abs->isa<AbstractTuple>()) {
          // If no elements changed and it is an AbstractTuple, do not convert.
          return nullptr;
        }
        // If no elements changed but it is not an AbstractTuple, convert it by copy elements.
        return std::make_shared<AbstractTuple>(seq_elements);
      }
      // Always make new AbstractTuple when elements changed.
      std::vector<AbstractBasePtr> elements;
      elements.reserve(seq_elements.size());
      for (const auto &element : seq_elements) {
        auto iter = changed_elements.find(element);
        if (iter != changed_elements.end()) {
          (void)elements.emplace_back(iter->second);
        } else {
          (void)elements.emplace_back(element);
        }
      }
      return std::make_shared<AbstractTuple>(std::move(elements));
    }
    // AbstractDict --> AbstractTuple.
    auto abs_dict = abs->cast<AbstractDictionaryPtr>();
    if (abs_dict != nullptr) {
      const auto &dict_elements = abs_dict->elements();
      std::vector<AbstractBasePtr> elements;
      elements.reserve(dict_elements.size());
      for (const auto &element : dict_elements) {
        auto new_element = ConvertToAbstractTuple(element.second, depth + 1);
        if (new_element != nullptr) {
          (void)elements.emplace_back(new_element);
        } else {
          (void)elements.emplace_back(element.second);
        }
      }
      return std::make_shared<AbstractTuple>(elements);
    }
    // AbstractCOOTensor --> AbstractTuple.
    auto abs_sparse = abs->cast<abstract::AbstractCOOTensorPtr>();
    if (abs_sparse != nullptr) {
      std::vector<AbstractBasePtr> elements{abs_sparse->indices(), abs_sparse->values(), abs_sparse->dense_shape()};
      return std::make_shared<AbstractTuple>(std::move(elements));
    }
    // AbstractRowTensor --> AbstractTuple.
    auto abs_row_tensor = abs->cast<std::shared_ptr<AbstractRowTensor>>();
    if (abs_row_tensor != nullptr) {
      std::vector<AbstractBasePtr> elements{abs_row_tensor->indices(), abs_row_tensor->values(),
                                            abs_row_tensor->dense_shape()};
      return std::make_shared<AbstractTuple>(std::move(elements));
    }
    return nullptr;
  }

  AbstractBasePtr ConvertAbstract(const AbstractBasePtr &abs) override {
    // AbstractSequence, AbstractDict, AbstractCOOTensor, AbstractRowTensor --> AbstractTuple.
    return ConvertToAbstractTuple(abs, 0);
  }
};
}  // namespace

bool SimplifyDataStructures(const FuncGraphPtr &root, const FuncGraphManagerPtr &manager) {
  MS_EXCEPTION_IF_NULL(manager);
  manager->AddFuncGraph(root);
  SimplifyDataStructuresRewriter rewriter(root, manager);
  return rewriter.Execute();
}

bool CleanAfterOptA(const FuncGraphPtr &root, const FuncGraphManagerPtr &manager) {
  MS_EXCEPTION_IF_NULL(manager);
  manager->AddFuncGraph(root);
  CleanAfterOptARewriter rewriter(root, manager);
  return rewriter.Execute();
}
}  // namespace opt
}  // namespace mindspore
