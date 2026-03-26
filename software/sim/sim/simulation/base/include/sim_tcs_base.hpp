/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_tcs_base {                                                                                                                    

constexpr std::size_t k_param_count = 8U;                                                                                                   
constexpr std::size_t k_local_param_count = 0U;                                                                                             
constexpr std::size_t k_isim_param_count = 8U;                                                                                              

inline std::uint8_t P_TCSPWR = 0;                                                                                                           
inline std::uint8_t P_TCSPSS = 0;                                                                                                           
inline std::uint8_t P_TCSTMS = 0;                                                                                                           
inline std::uint8_t P_TCSMNS = 0;                                                                                                           
inline std::uint8_t P_TCSLS = 0;                                                                                                            
inline std::int8_t P_TCSCLOCK = 0;                                                                                                          
inline std::int16_t P_TCSFRQ = 0;                                                                                                           
inline float P_TCST = 0;                                                                                                                    

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
  sim_base::param_entry_t{31, "TCSPWR", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSPWR), 0},                          
  sim_base::param_entry_t{32, "TCSPSS", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSPSS), 0},                          
  sim_base::param_entry_t{33, "TCSTMS", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSTMS), 0},                          
  sim_base::param_entry_t{34, "TCSMNS", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSMNS), 0},                          
  sim_base::param_entry_t{35, "TCSLS", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSLS), 0},                            
  sim_base::param_entry_t{36, "TCSCLOCK", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TCSCLOCK), 0},                         
  sim_base::param_entry_t{37, "TCSFRQ", 16U, sim_base::physical_type_t::int16, static_cast<void*>(&P_TCSFRQ), 0},                           
  sim_base::param_entry_t{38, "TCST", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_TCST), 0}                              
};                                                                                                                                          

inline std::array<sim_base::param_entry_t, k_local_param_count> LOCAL_PARAMS = {                                                            
};                                                                                                                                          

inline std::array<sim_base::param_entry_t, k_isim_param_count> ISIM_PARAMS = {                                                              
  sim_base::param_entry_t{31, "TCSPWR", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSPWR), 0},                          
  sim_base::param_entry_t{32, "TCSPSS", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSPSS), 0},                          
  sim_base::param_entry_t{33, "TCSTMS", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSTMS), 0},                          
  sim_base::param_entry_t{34, "TCSMNS", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSMNS), 0},                          
  sim_base::param_entry_t{35, "TCSLS", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TCSLS), 0},                            
  sim_base::param_entry_t{36, "TCSCLOCK", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TCSCLOCK), 0},                         
  sim_base::param_entry_t{37, "TCSFRQ", 16U, sim_base::physical_type_t::int16, static_cast<void*>(&P_TCSFRQ), 0},                           
  sim_base::param_entry_t{38, "TCST", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_TCST), 0}                              
};                                                                                                                                          

inline void init_defaults() {                                                                                                               
  sim_base::init_params(PARAMS);                                                                                                            
  sim_base::init_params(LOCAL_PARAMS);                                                                                                      
}                                                                                                                                           

}  // namespace sim_tcs_base                                                                                                                
