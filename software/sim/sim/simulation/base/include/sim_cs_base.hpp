/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_cs_base {                                                                                                                     

constexpr std::size_t k_param_count = 3U;                                                                                                   

extern std::uint8_t P_CSPWR;                                                                                                                
extern std::int8_t P_CSCLOCK;                                                                                                               
extern sim_base::float16_t P_CST;                                                                                                           

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
  sim_base::param_entry_t{0, "CSPWR", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_CSPWR)},                                
  sim_base::param_entry_t{1, "CSCLOCK", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_CSCLOCK)},                             
  sim_base::param_entry_t{2, "CST", 16U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_CST)}                                   
};                                                                                                                                          

}  // namespace sim_cs_base                                                                                                                 
