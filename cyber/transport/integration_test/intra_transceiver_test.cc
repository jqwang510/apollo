/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
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
 *****************************************************************************/

#include "cyber/transport/transmitter/intra_transmitter.h"

#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"

#include "cyber/proto/unit_test.pb.h"
#include "cyber/transport/receiver/intra_receiver.h"

namespace apollo {
namespace cyber {
namespace transport {

class IntraTransceiverTest : public ::testing::Test {
 protected:
  using TransmitterPtr = std::shared_ptr<Transmitter<proto::UnitTest>>;
  using ReceiverPtr = std::shared_ptr<Receiver<proto::UnitTest>>;

  IntraTransceiverTest() : channel_name_("intra_channel") {}

  virtual ~IntraTransceiverTest() {}

  virtual void SetUp() {
    RoleAttributes attr;
    attr.set_channel_name(channel_name_);
    transmitter_a_ = std::make_shared<IntraTransmitter<proto::UnitTest>>(attr);
    transmitter_b_ = std::make_shared<IntraTransmitter<proto::UnitTest>>(attr);

    transmitter_a_->Enable();
    transmitter_b_->Enable();
  }

  virtual void TearDown() {
    transmitter_a_ = nullptr;
    transmitter_b_ = nullptr;
  }

  std::string channel_name_;
  TransmitterPtr transmitter_a_ = nullptr;
  TransmitterPtr transmitter_b_ = nullptr;
};

TEST_F(IntraTransceiverTest, constructor) {
  RoleAttributes attr;
  TransmitterPtr transmitter =
      std::make_shared<IntraTransmitter<proto::UnitTest>>(attr);
  ReceiverPtr receiver =
      std::make_shared<IntraReceiver<proto::UnitTest>>(attr, nullptr);

  EXPECT_EQ(transmitter->seq_num(), 0);

  auto& transmitter_id = transmitter->id();
  auto& receiver_id = receiver->id();

  EXPECT_NE(transmitter_id.ToString(), receiver_id.ToString());
}

TEST_F(IntraTransceiverTest, enable_and_disable) {
  // repeated call
  transmitter_a_->Enable();

  std::vector<proto::UnitTest> msgs;
  RoleAttributes attr;
  attr.set_channel_name(channel_name_);
  ReceiverPtr receiver = std::make_shared<IntraReceiver<proto::UnitTest>>(
      attr, [&msgs](const std::shared_ptr<proto::UnitTest>& msg,
                    const MessageInfo& msg_info, const RoleAttributes& attr) {
        (void)msg_info;
        (void)attr;
        msgs.emplace_back(*msg);
      });

  receiver->Enable();
  // repeated call
  receiver->Enable();

  ReceiverPtr receiver_null_cb =
      std::make_shared<IntraReceiver<proto::UnitTest>>(attr, nullptr);
  receiver_null_cb->Enable();

  auto msg = std::make_shared<proto::UnitTest>();
  msg->set_class_name("IntraTransceiverTest");
  msg->set_case_name("enable_and_disable");

  EXPECT_TRUE(transmitter_a_->Transmit(msg));
  EXPECT_EQ(msgs.size(), 1);

  EXPECT_TRUE(transmitter_b_->Transmit(msg));
  EXPECT_EQ(msgs.size(), 2);

  for (auto& item : msgs) {
    EXPECT_EQ(item.class_name(), "IntraTransceiverTest");
    EXPECT_EQ(item.case_name(), "enable_and_disable");
  }

  transmitter_b_->Disable(receiver->attributes());
  EXPECT_FALSE(transmitter_b_->Transmit(msg));

  transmitter_b_->Enable(receiver->attributes());
  auto& transmitter_b_attr = transmitter_b_->attributes();

  receiver->Disable();
  receiver->Enable(transmitter_b_attr);

  msgs.clear();
  EXPECT_TRUE(transmitter_a_->Transmit(msg));
  EXPECT_EQ(msgs.size(), 0);

  EXPECT_TRUE(transmitter_b_->Transmit(msg));
  EXPECT_EQ(msgs.size(), 1);
  for (auto& item : msgs) {
    EXPECT_EQ(item.class_name(), "IntraTransceiverTest");
    EXPECT_EQ(item.case_name(), "enable_and_disable");
  }

  receiver->Disable(transmitter_b_attr);
  msgs.clear();
  EXPECT_TRUE(transmitter_b_->Transmit(msg));
  EXPECT_EQ(msgs.size(), 0);
}

}  // namespace transport
}  // namespace cyber
}  // namespace apollo
