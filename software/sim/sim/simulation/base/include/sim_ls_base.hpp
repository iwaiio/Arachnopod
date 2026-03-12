/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_ls_base {                                                                                                                     

constexpr std::size_t k_param_count = 3U;                                                                                                   

extern std::uint8_t P_LSPWR;                                                                                                                
extern std::int8_t P_LSCLOCK;                                                                                                               
extern sim_base::float16_t P_LST;                                                                                                           

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
  sim_base::param_entry_t{49, "LSPWR", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_LSPWR)},                               
  sim_base::param_entry_t{50, "LSCLOCK", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_LSCLOCK)},                            
  sim_base::param_entry_t{51, "LST", 16U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_LST)}                                  
};                                                                                                                                          

}  // namespace sim_ls_base                                                                                                                 
