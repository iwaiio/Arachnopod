#include "cs_scheduler.hpp"

#include <sstream>

#include "cs_scheduler_internal.hpp"
#include "i_sys_algo.hpp"

namespace cs::scheduler {

void init(const context_t& ctx) {
  internal::G_CTX = ctx;
  internal::init_node_layouts();
  internal::G_READY = true;
}

void reset() {
  internal::G_TASKS = {};
  internal::G_NORMAL_QUEUE.clear();
  internal::G_PRIORITY_QUEUE.clear();
  internal::G_COMPLETIONS.clear();
  internal::G_PENDING_CMD = {};
  internal::G_ACTIVE_TASK_SLOT = internal::k_invalid_slot;
  internal::G_NEXT_TASK_ID = 1U;
  internal::G_PRIORITY_BURST = 0U;
}

task_id_t enqueue_exchange(const control::target_t target,
                           const queue_class_t queue,
                           const bool periodic) {
  const auto* node = internal::find_node_by_target(target);
  if (node == nullptr) {
    return internal::k_invalid_task_id;
  }

  return internal::add_task(*node, internal::task_kind_t::data_request, internal::to_task_queue(queue), periodic);
}

task_id_t ensure_periodic_exchange(const control::target_t target, const queue_class_t queue) {
  const auto* node = internal::find_node_by_target(target);
  if (node == nullptr) {
    return internal::k_invalid_task_id;
  }

  const std::size_t existing_slot = internal::find_periodic_data_task_slot(*node);
  if (existing_slot != internal::k_invalid_slot) {
    auto& task = internal::G_TASKS[existing_slot];
    task.enabled = true;
    internal::update_task_queue_class(existing_slot, internal::to_task_queue(queue));
    return task.id;
  }

  return internal::add_task(*node, internal::task_kind_t::data_request, internal::to_task_queue(queue), true);
}

task_id_t enqueue_command(const control::target_t target,
                          const std::uint16_t command_id,
                          const float command_value,
                          const bool has_value,
                          const queue_class_t queue) {
  const auto* node = internal::find_node_by_target(target);
  if (node == nullptr) {
    return internal::k_invalid_task_id;
  }
  if (!internal::command_matches_node(*node, command_id)) {
    std::ostringstream oss;
    oss << "cs scheduler: rejected command id=" << command_id << " for target=" << node->name;
    internal::log_warning(oss.str());
    return internal::k_invalid_task_id;
  }

  return internal::add_task(*node,
                            internal::task_kind_t::command,
                            internal::to_task_queue(queue),
                            false,
                            command_id,
                            command_value,
                            has_value);
}

bool cancel_periodic_exchange(const control::target_t target) {
  const auto* node = internal::find_node_by_target(target);
  if (node == nullptr) {
    return false;
  }

  const std::size_t slot = internal::find_periodic_data_task_slot(*node);
  if (slot == internal::k_invalid_slot) {
    return false;
  }

  auto& task = internal::G_TASKS[slot];
  if (slot == internal::G_ACTIVE_TASK_SLOT) {
    task.enabled = false;
    task.periodic = false;
    return true;
  }

  internal::remove_slot_from_queues(slot);
  internal::clear_task_slot(slot);
  return true;
}

bool cancel_task(const task_id_t task_id) {
  const std::size_t slot = internal::find_task_slot_by_id(task_id);
  if (slot == internal::k_invalid_slot) {
    return false;
  }

  auto& task = internal::G_TASKS[slot];
  if (slot == internal::G_ACTIVE_TASK_SLOT) {
    task.enabled = false;
    task.periodic = false;
    return true;
  }

  internal::remove_slot_from_queues(slot);
  internal::clear_task_slot(slot);
  return true;
}

std::size_t cancel_target(const control::target_t target) {
  const auto* node = internal::find_node_by_target(target);
  if (node == nullptr) {
    return 0U;
  }

  const auto removed = internal::remove_tasks_for_node(*node);
  internal::clear_pending_command_window(*node);
  internal::clear_command_buffer_window(*node);
  internal::clear_command_msg_window(*node);
  return removed;
}

bool try_pop_completion(completion_t& out) {
  if (internal::G_COMPLETIONS.empty()) {
    return false;
  }

  out = internal::G_COMPLETIONS.front();
  internal::G_COMPLETIONS.pop_front();
  return true;
}

void exchange_control(isysalgo::bus_state_t& bus) {
  if (!internal::G_READY) {
    return;
  }

  const auto completion = internal::finalize_active_task(bus);
  if (completion.valid) {
    internal::G_COMPLETIONS.push_back(completion_t{
        .valid = true,
        .target = completion.target,
        .kind = internal::to_public_completion_kind(completion.kind),
        .periodic = completion.periodic,
        .have_result = completion.have_result,
        .result = internal::to_public_completion_result(completion.result),
    });

    std::ostringstream oss;
    oss << "cs scheduler: finish task id=" << completion.id << " target=" << completion.target_name
        << " kind=" << internal::task_kind_to_string(completion.kind)
        << " result=" << isysalgo::IBUS_RESULT_TO_STRING(completion.result);
    if (!completion.have_result) {
      oss << " pending_result=0";
    }
    internal::log_info(oss.str());
    return;
  }

  if (internal::G_ACTIVE_TASK_SLOT != internal::k_invalid_slot) {
    return;
  }
  internal::activate_next_task();
}

}  // namespace cs::scheduler
