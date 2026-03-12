/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_mns_base {                                                                                                                    

constexpr std::size_t k_param_count = 3U;                                                                                                   

extern std::uint8_t P_MNSPWR;                                                                                                               
extern std::int8_t P_MNSCLOCK;                                                                                                              
extern sim_base::float16_t P_MNST;                                                                                                          

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
  sim_base::param_entry_t{46, "MNSPWR", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_MNSPWR)},                             
  sim_base::param_entry_t{47, "MNSCLOCK", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_MNSCLOCK)},                          
  sim_base::param_entry_t{48, "MNST", 16U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_MNST)}                                
};                                                                                                                                          

}  // namespace sim_mns_base                                                                                                                
