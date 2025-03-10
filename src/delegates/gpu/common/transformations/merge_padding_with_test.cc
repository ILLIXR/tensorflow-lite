/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "delegates/gpu/common/transformations/merge_padding_with.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/types/any.h"
#include "delegates/gpu/common/data_type.h"
#include "delegates/gpu/common/model.h"
#include "delegates/gpu/common/model_transformer.h"
#include "delegates/gpu/common/operations.h"
#include "delegates/gpu/common/shape.h"
#include "delegates/gpu/common/tensor.h"

namespace tflite {
namespace gpu {
namespace {

TEST(MergePaddingWith, Smoke) {
  GraphFloat32 graph;
  auto input = graph.NewValue();

  auto pad_node = graph.NewNode();
  ASSERT_TRUE(graph.AddConsumer(pad_node->id, input->id).ok());
  pad_node->operation.type = ToString(OperationType::PAD);
  PadAttributes attr;
  attr.prepended = BHWC(0, 1, 1, 0);
  attr.appended = BHWC(0, 2, 2, 0);
  pad_node->operation.attributes = attr;

  auto conv_node = graph.NewNode();
  Value* temp = nullptr;
  ASSERT_TRUE(ConnectTwoNodes(&graph, pad_node, conv_node, &temp).ok());
  ASSERT_TRUE(AddOutput(&graph, conv_node, &temp).ok());
  conv_node->operation.type = ToString(OperationType::CONVOLUTION_2D);
  Convolution2DAttributes conv_attr;
  conv_attr.padding.appended = HW(0, 0);
  conv_attr.padding.prepended = HW(0, 0);
  conv_node->operation.attributes = conv_attr;

  ASSERT_EQ(2, graph.nodes().size());

  auto transformation = NewMergePaddingWithConvolution2D();
  ModelTransformer transformer(&graph);
  transformer.Apply("merge_padding", transformation.get());

  ASSERT_EQ(1, graph.nodes().size());
  ASSERT_EQ(2, graph.values().size());
  ASSERT_EQ(conv_node, graph.nodes()[0]);
  conv_attr =
      absl::any_cast<Convolution2DAttributes>(conv_node->operation.attributes);
  EXPECT_EQ(HW(1, 1), conv_attr.padding.prepended);
  EXPECT_EQ(HW(2, 2), conv_attr.padding.appended);
}

TEST(MergePaddingWith, MergeTwo) {
  GraphFloat32 graph;
  auto input = graph.NewValue();

  auto pad_node1 = graph.NewNode();
  ASSERT_TRUE(graph.AddConsumer(pad_node1->id, input->id).ok());
  pad_node1->operation.type = ToString(OperationType::PAD);
  PadAttributes attr;
  attr.prepended = BHWC(0, 1, 1, 0);
  attr.appended = BHWC(0, 0, 0, 0);
  pad_node1->operation.attributes = attr;

  auto pad_node2 = graph.NewNode();
  Value* temp1 = nullptr;
  ASSERT_TRUE(ConnectTwoNodes(&graph, pad_node1, pad_node2, &temp1).ok());
  pad_node2->operation.type = ToString(OperationType::PAD);
  attr.prepended = BHWC(0, 0, 0, 0);
  attr.appended = BHWC(0, 2, 2, 0);
  pad_node2->operation.attributes = attr;

  auto conv_node = graph.NewNode();
  Value* temp2 = nullptr;
  ASSERT_TRUE(ConnectTwoNodes(&graph, pad_node2, conv_node, &temp2).ok());
  ASSERT_TRUE(AddOutput(&graph, conv_node, &temp2).ok());
  conv_node->operation.type = ToString(OperationType::CONVOLUTION_2D);
  Convolution2DAttributes conv_attr;
  conv_attr.padding.appended = HW(0, 0);
  conv_attr.padding.prepended = HW(0, 0);
  conv_node->operation.attributes = conv_attr;

  ASSERT_EQ(3, graph.nodes().size());

  auto transformation = NewMergePaddingWithConvolution2D();
  ModelTransformer transformer(&graph);
  transformer.Apply("merge_padding", transformation.get());

  ASSERT_EQ(1, graph.nodes().size());
  ASSERT_EQ(2, graph.values().size());
  ASSERT_EQ(conv_node, graph.nodes()[0]);
  conv_attr =
      absl::any_cast<Convolution2DAttributes>(conv_node->operation.attributes);
  EXPECT_EQ(HW(1, 1), conv_attr.padding.prepended);
  EXPECT_EQ(HW(2, 2), conv_attr.padding.appended);
}

TEST(MergePaddingWithAdd, MergeAlignedPadding) {
  GraphFloat32 graph;
  auto input0 = graph.NewValue();
  input0->tensor.shape = BHWC(1, 4, 4, 8);
  auto input1 = graph.NewValue();
  auto padded = graph.NewValue();
  auto output = graph.NewValue();

  auto pad_node = graph.NewNode();
  pad_node->operation.type = ToString(OperationType::PAD);
  PadAttributes pad_attr;
  pad_attr.prepended = BHWC(0, 0, 0, 0);
  pad_attr.appended = BHWC(0, 0, 0, 32);
  pad_node->operation.attributes = pad_attr;

  ASSERT_TRUE(graph.AddConsumer(pad_node->id, input0->id).ok());
  ASSERT_TRUE(graph.SetProducer(pad_node->id, padded->id).ok());

  auto add_node = graph.NewNode();
  ElementwiseAttributes add_attr;
  ASSERT_TRUE(graph.AddConsumer(add_node->id, padded->id).ok());
  ASSERT_TRUE(graph.AddConsumer(add_node->id, input1->id).ok());
  ASSERT_TRUE(graph.SetProducer(add_node->id, output->id).ok());
  add_node->operation.type = ToString(OperationType::ADD);
  add_node->operation.attributes = add_attr;

  ASSERT_EQ(2, graph.nodes().size());
  ASSERT_EQ(4, graph.values().size());

  auto transformation = NewMergePaddingWithAdd();
  ModelTransformer transformer(&graph);
  transformer.Apply("merge_padding", transformation.get());

  ASSERT_EQ(1, graph.nodes().size());
  ASSERT_EQ(3, graph.values().size());
  EXPECT_EQ(add_node, graph.nodes()[0]);
}

TEST(MergePaddingWithAdd, DoNotTrigger_AddWithAttributes) {
  GraphFloat32 graph;
  auto input0 = graph.NewValue();
  input0->tensor.shape = BHWC(1, 4, 4, 8);
  auto input1 = graph.NewValue();
  auto padded = graph.NewValue();
  auto output = graph.NewValue();

  auto pad_node = graph.NewNode();
  pad_node->operation.type = ToString(OperationType::PAD);
  PadAttributes pad_attr;
  pad_attr.prepended = BHWC(0, 0, 0, 0);
  pad_attr.appended = BHWC(0, 0, 0, 32);
  pad_node->operation.attributes = pad_attr;

  ASSERT_TRUE(graph.AddConsumer(pad_node->id, input0->id).ok());
  ASSERT_TRUE(graph.SetProducer(pad_node->id, padded->id).ok());

  auto add_node = graph.NewNode();
  ElementwiseAttributes add_attr;
  add_attr.param = Tensor<HWC, DataType::FLOAT32>();
  ASSERT_TRUE(graph.AddConsumer(add_node->id, padded->id).ok());
  ASSERT_TRUE(graph.AddConsumer(add_node->id, input1->id).ok());
  ASSERT_TRUE(graph.SetProducer(add_node->id, output->id).ok());
  add_node->operation.type = ToString(OperationType::ADD);
  add_node->operation.attributes = add_attr;

  ASSERT_EQ(2, graph.nodes().size());
  ASSERT_EQ(4, graph.values().size());

  auto transformation = NewMergePaddingWithAdd();
  ModelTransformer transformer(&graph);
  transformer.Apply("merge_padding", transformation.get());

  ASSERT_EQ(2, graph.nodes().size());
  ASSERT_EQ(4, graph.values().size());
  EXPECT_EQ(pad_node, graph.nodes()[0]);
  EXPECT_EQ(add_node, graph.nodes()[1]);
}

}  // namespace
}  // namespace gpu
}  // namespace tflite
