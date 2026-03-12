/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_tcs_base {                                                                                                                    

constexpr std::size_t k_param_count = 8U;                                                                                                   

extern std::uint8_t P_TCSPWR;                                                                                                               
extern std::uint8_t P_TCSPSS;                                                                                                               
extern std::uint8_t P_TCSTMS;                                                                                                               
extern std::uint8_t P_TCSMNS;                                                                                                               
extern std::uint8_t P_TCSLS;                                                                                                                
extern std::int8_t P_TCSCLOCK;                                                                                                              
extern std::int16_t P_TCSFRQ;                                                                                                               
extern sim_base::float16_t P_TCST;                                                                                                          

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
  sim_base::param_entry_t{31, "TCSPWR", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_TCSPWR)},                             
  sim_base::param_entry_t{32, "TCSPSS", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_TCSPSS)},                             
  sim_base::param_entry_t{33, "TCSTMS", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_TCSTMS)},                             
  sim_base::param_entry_t{34, "TCSMNS", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_TCSMNS)},                             
  sim_base::param_entry_t{35, "TCSLS", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_TCSLS)},                               
  sim_base::param_entry_t{36, "TCSCLOCK", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_TCSCLOCK)},                          
  sim_base::param_entry_t{37, "TCSFRQ", 16U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_TCSFRQ)},                             
  sim_base::param_entry_t{38, "TCST", 16U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_TCST)}                                
};                                                                                                                                          

}  // namespace sim_tcs_base                                                                                                                
