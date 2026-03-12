/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_tms_base {                                                                                                                    

constexpr std::size_t k_param_count = 7U;                                                                                                   

extern std::uint8_t P_TMSPWR;                                                                                                               
extern std::int8_t P_TMSCLOCK;                                                                                                              
extern std::int8_t P_TMSPWRFAN1;                                                                                                            
extern std::int8_t P_TMSPWRFAN2;                                                                                                            
extern std::int8_t P_TMSPWRFAN3;                                                                                                            
extern std::int8_t P_TMSPWRFAN4;                                                                                                            
extern sim_base::float16_t P_TMST;                                                                                                          

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
  sim_base::param_entry_t{39, "TMSPWR", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_TMSPWR)},                             
  sim_base::param_entry_t{40, "TMSCLOCK", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_TMSCLOCK)},                          
  sim_base::param_entry_t{41, "TMSPWRFAN1", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_TMSPWRFAN1)},                      
  sim_base::param_entry_t{42, "TMSPWRFAN2", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_TMSPWRFAN2)},                      
  sim_base::param_entry_t{43, "TMSPWRFAN3", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_TMSPWRFAN3)},                      
  sim_base::param_entry_t{44, "TMSPWRFAN4", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_TMSPWRFAN4)},                      
  sim_base::param_entry_t{45, "TMST", 16U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_TMST)}                                
};                                                                                                                                          

}  // namespace sim_tms_base                                                                                                                
