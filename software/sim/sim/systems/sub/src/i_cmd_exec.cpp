#include "i_cmd_exec.hpp"

namespace icmdexec {
namespace {

void call_status_hook(status_hook_t status_hook,
                      void* user,
                      const binding_t& binding,
                      const isim::status_t status) {
  if (status_hook != nullptr) {
    status_hook(user, binding, status);
  }
}

bool apply_bool_pair(const ipar::context_t& ctx,
                     const binding_t& binding,
                     const status_hook_t status_hook,
                     void* user) {
  bool changed = false;

  if (icmd::ICMDACT(ctx, binding.cmd_id)) {
    const auto status = isim::ISIMSETI32(binding.param_id, 1);
    if (status != isim::status_t::ok) {
      call_status_hook(status_hook, user, binding, status);
    } else {
      changed = true;
    }
  }

  if (icmd::ICMDACT(ctx, binding.alt_cmd_id)) {
    const auto status = isim::ISIMSETI32(binding.param_id, 0);
    if (status != isim::status_t::ok) {
      call_status_hook(status_hook, user, binding, status);
    } else {
      changed = true;
    }
  }

  return changed;
}

bool apply_nonzero_value(const ipar::context_t& ctx,
                         const binding_t& binding,
                         const status_hook_t status_hook,
                         void* user) {
  const std::int32_t value = icmd::ICMDVAL(ctx, binding.cmd_id, 0);
  if (value == 0) {
    return false;
  }

  const auto status = isim::ISIMSETI32(binding.param_id, value);
  if (status != isim::status_t::ok) {
    call_status_hook(status_hook, user, binding, status);
    return false;
  }

  return true;
}

}  // namespace

bool apply(const ipar::context_t& ctx,
           const binding_t& binding,
           const status_hook_t status_hook,
           void* user) {
  switch (binding.mode) {
    case mode_t::bool_pair:
      return apply_bool_pair(ctx, binding, status_hook, user);
    case mode_t::nonzero_value:
      return apply_nonzero_value(ctx, binding, status_hook, user);
    default:
      return false;
  }
}

std::size_t apply_all(const ipar::context_t& ctx,
                      const binding_t* bindings,
                      const std::size_t count,
                      const status_hook_t status_hook,
                      void* user) {
  if ((bindings == nullptr) || (count == 0U)) {
    return 0U;
  }

  std::size_t applied = 0U;
  for (std::size_t i = 0; i < count; ++i) {
    if (apply(ctx, bindings[i], status_hook, user)) {
      ++applied;
    }
  }

  return applied;
}

}  // namespace icmdexec
