#pragma once

#include <chrono>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>

namespace cmr_clients_utils {

template<class ActionT>
class BasicActionClient {
  
  public:

  BasicActionClient(std::string client_node_name, std::string action_name, 
                      unsigned int action_timeout_ms = 100)
  {
    client_node_ = rclcpp::Node::make_shared(client_node_name);
    action_client_ = rclcpp_action::create_client<ActionT>(client_node_, action_name);
    
    action_timeout_ = std::chrono::milliseconds(action_timeout_ms);
    action_name_ = action_name;
    client_node_name_ = client_node_name;
  }

  bool is_server_ready()
  {
    if (!rclcpp::ok())
    {
      RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to check if action server is responding: ROS isn't running");
      return false;
    }
    return action_client_->action_server_is_ready();
  }

  rclcpp_action::ResultCode send_goal(const typename ActionT::Goal & goal)
  {
    if (!action_client_->wait_for_action_server(action_timeout_))
    {
      RCLCPP_ERROR(rclcpp::get_logger(client_node_name_), "Action server did not respond in time: failed to send goal");
      return rclcpp_action::ResultCode::ABORTED;
    }

    auto result_future = action_client_->async_send_goal(goal);

    if (rclcpp::spin_until_future_complete(this->client_node_, result_future) ==
        rclcpp::FutureReturnCode::SUCCESS)
    {
      RCLCPP_INFO(rclcpp::get_logger(client_node_name_), "Successfully called action!");
    } else {
      RCLCPP_ERROR(rclcpp::get_logger(client_node_name_), "Failed to call action.");
      return rclcpp_action::ResultCode::ABORTED;
    }

    action_goal_handle_ = result_future.get();

    if (!action_goal_handle_) {
      RCLCPP_ERROR(rclcpp::get_logger(client_node_name_), "Goal was rejected by server");
      return rclcpp_action::ResultCode::ABORTED;
    }

    auto result_code_future = action_client_->async_get_result(action_goal_handle_);

    if (rclcpp::spin_until_future_complete(this->client_node_, result_code_future) !=
        rclcpp::FutureReturnCode::SUCCESS)
    {
      RCLCPP_ERROR(rclcpp::get_logger(client_node_name_), "Failed to call action.");
      return rclcpp_action::ResultCode::ABORTED;
    }
    
    return result_code_future.get().code;
  }

  void cancel_goal() 
  {
    auto future_cancel = action_client_->async_cancel_goal(action_goal_handle_);
    if (rclcpp::spin_until_future_complete(this->client_node_, future_cancel) !=
      rclcpp::FutureReturnCode::SUCCESS)
    {
      RCLCPP_ERROR(rclcpp::get_logger(client_node_name_), "Failed to cancel action.");
      return;
    }

    RCLCPP_INFO(rclcpp::get_logger(client_node_name_), "Successfully requested to cancel goal.");
  }

  private:

  typename rclcpp_action::Client<ActionT>::SharedPtr action_client_;
  rclcpp::Node::SharedPtr client_node_;
  std::shared_ptr<rclcpp_action::ClientGoalHandle<ActionT>> action_goal_handle_;
  std::string action_name_;
  std::chrono::milliseconds action_timeout_;
  std::string client_node_name_;
};

}