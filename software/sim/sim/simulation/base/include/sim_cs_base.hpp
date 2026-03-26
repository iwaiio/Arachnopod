/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_cs_base {                                                                                                                     

constexpr std::size_t k_param_count = 0U;                                                                                                   
constexpr std::size_t k_local_param_count = 3U;                                                                                             
constexpr std::size_t k_isim_param_count = 3U;                                                                                              

inline std::uint8_t P_LCSPWR = 1;                                                                                                           
inline std::int8_t P_LCSCLOCK = 0;                                                                                                          
inline float P_LCST = 0;                                                                                                                    

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
};                                                                                                                                          

inline std::array<sim_base::param_entry_t, k_local_param_count> LOCAL_PARAMS = {                                                            
  sim_base::param_entry_t{32768, "LCSPWR", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_LCSPWR), 1},                       
  sim_base::param_entry_t{32769, "LCSCLOCK", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_LCSCLOCK), 0},                      
  sim_base::param_entry_t{32770, "LCST", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_LCST), 0}                           
};                                                                                                                                          

inline std::array<sim_base::param_entry_t, k_isim_param_count> ISIM_PARAMS = {                                                              
  sim_base::param_entry_t{32768, "LCSPWR", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_LCSPWR), 1},                       
  sim_base::param_entry_t{32769, "LCSCLOCK", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_LCSCLOCK), 0},                      
  sim_base::param_entry_t{32770, "LCST", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_LCST), 0}                           
};                                                                                                                                          

inline void init_defaults() {                                                                                                               
  sim_base::init_params(PARAMS);                                                                                                            
  sim_base::init_params(LOCAL_PARAMS);                                                                                                      
}                                                                                                                                           

}  // namespace sim_cs_base                                                                                                                 
