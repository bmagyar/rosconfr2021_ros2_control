// Copyright (c) 2021, Bence Magyar and Denis Stogl
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "test_rrbot_controller.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

// When there are many mandatory parameters, set all by default and remove one by one in a
// parameterized test
TEST_P(RRBotControllerTestParameterizedParameters, one_parameter_is_missing)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_ERROR);
}

// TODO(anyone): the new gtest version after 1.8.0 uses INSTANTIATE_TEST_SUITE_P
INSTANTIATE_TEST_CASE_P(
  MissingMandatoryParameterDuringConfiguration, RRBotControllerTestParameterizedParameters,
  ::testing::Values(
    std::make_tuple(std::string("joints"), rclcpp::ParameterValue(std::vector<std::string>())),
    std::make_tuple(std::string("interface_name"), rclcpp::ParameterValue(""))));

TEST_F(RRBotControllerTest, joint_names_parameter_not_set)
{
  SetUpController(false);

  ASSERT_TRUE(controller_->joint_names_.empty());
  ASSERT_TRUE(controller_->interface_name_.empty());

  controller_->get_node()->set_parameter({"interface_name", interface_name_});

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_ERROR);

  ASSERT_TRUE(controller_->joint_names_.empty());
  ASSERT_TRUE(controller_->interface_name_.empty());
}

TEST_F(RRBotControllerTest, interface_parameter_not_set)
{
  SetUpController(false);

  controller_->get_node()->set_parameter({"joints", joint_names_});

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_ERROR);

  ASSERT_TRUE(!controller_->joint_names_.empty());
  ASSERT_TRUE(controller_->interface_name_.empty());
}

TEST_F(RRBotControllerTest, all_parameters_set_configure_success)
{
  SetUpController();

  ASSERT_TRUE(controller_->joint_names_.empty());
  ASSERT_TRUE(controller_->interface_name_.empty());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);

  ASSERT_TRUE(!controller_->joint_names_.empty());
  ASSERT_TRUE(controller_->joint_names_.size() == joint_names_.size());
  ASSERT_TRUE(std::equal(
    controller_->joint_names_.begin(), controller_->joint_names_.end(), joint_names_.begin(),
    joint_names_.end()));

  ASSERT_TRUE(!controller_->interface_name_.empty());
  ASSERT_TRUE(controller_->interface_name_ == interface_name_);
}

TEST_F(RRBotControllerTest, check_intefaces)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);

  auto command_intefaces = controller_->command_interface_configuration();
  ASSERT_EQ(command_intefaces.names.size(), joint_command_values_.size());

  auto state_intefaces = controller_->state_interface_configuration();
  ASSERT_EQ(state_intefaces.names.size(), joint_state_values_.size());
}

TEST_F(RRBotControllerTest, activate_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
}

TEST_F(RRBotControllerTest, update_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->update(), controller_interface::return_type::OK);
}

TEST_F(RRBotControllerTest, deactivate_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_deactivate(rclcpp_lifecycle::State()), NODE_SUCCESS);
}

TEST_F(RRBotControllerTest, reactivate_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_deactivate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->update(), controller_interface::return_type::OK);
}

TEST_F(RRBotControllerTest, publish_status_success)
{
  SetUpController();

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->update(), controller_interface::return_type::OK);

  ControllerStateMsg msg;
  subscribe_and_get_messages(msg);

  ASSERT_EQ(msg.set_point, 101.101);
}

TEST_F(RRBotControllerTest, receive_message_and_publish_updated_status)
{
  SetUpController();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(controller_->get_node()->get_node_base_interface());

  ASSERT_EQ(controller_->on_configure(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->on_activate(rclcpp_lifecycle::State()), NODE_SUCCESS);
  ASSERT_EQ(controller_->update(), controller_interface::return_type::OK);

  ControllerStateMsg msg;
  subscribe_and_get_messages(msg);

  ASSERT_EQ(msg.set_point, 101.101);

  publish_commands();
  ASSERT_TRUE(controller_->wait_for_commands(executor));

  ASSERT_EQ(controller_->update(), controller_interface::return_type::OK);

  EXPECT_EQ(joint_command_values_[0], 0.45);

  subscribe_and_get_messages(msg);

  ASSERT_EQ(msg.set_point, 0.45);
}
