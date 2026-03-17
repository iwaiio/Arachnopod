#pragma once

#include "i_control_console.hpp"

namespace cs::nodes {

void apply_periodic_policy(control::target_t target);
void pss_step();
void tcs_step();
void tms_step();
void mns_step();
void ls_step();

}  // namespace cs::nodes
