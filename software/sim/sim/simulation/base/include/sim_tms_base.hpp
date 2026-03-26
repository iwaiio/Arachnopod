/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_tms_base {                                                                                                                    

constexpr std::size_t k_param_count = 7U;                                                                                                   
constexpr std::size_t k_local_param_count = 0U;                                                                                             
constexpr std::size_t k_isim_param_count = 7U;                                                                                              

inline std::uint8_t P_TMSPWR = 0;                                                                                                           
inline std::int8_t P_TMSCLOCK = 0;                                                                                                          
inline std::int8_t P_TMSPWRFAN1 = 0;                                                                                                        
inline std::int8_t P_TMSPWRFAN2 = 0;                                                                                                        
inline std::int8_t P_TMSPWRFAN3 = 0;                                                                                                        
inline std::int8_t P_TMSPWRFAN4 = 0;                                                                                                        
inline float P_TMST = 0;                                                                                                                    

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
  sim_base::param_entry_t{39, "TMSPWR", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TMSPWR), 0},                          
  sim_base::param_entry_t{40, "TMSCLOCK", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSCLOCK), 0},                         
  sim_base::param_entry_t{41, "TMSPWRFAN1", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSPWRFAN1), 0},                     
  sim_base::param_entry_t{42, "TMSPWRFAN2", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSPWRFAN2), 0},                     
  sim_base::param_entry_t{43, "TMSPWRFAN3", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSPWRFAN3), 0},                     
  sim_base::param_entry_t{44, "TMSPWRFAN4", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSPWRFAN4), 0},                     
  sim_base::param_entry_t{45, "TMST", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_TMST), 0}                              
};                                                                                                                                          

inline std::array<sim_base::param_entry_t, k_local_param_count> LOCAL_PARAMS = {                                                            
};                                                                                                                                          

inline std::array<sim_base::param_entry_t, k_isim_param_count> ISIM_PARAMS = {                                                              
  sim_base::param_entry_t{39, "TMSPWR", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_TMSPWR), 0},                          
  sim_base::param_entry_t{40, "TMSCLOCK", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSCLOCK), 0},                         
  sim_base::param_entry_t{41, "TMSPWRFAN1", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSPWRFAN1), 0},                     
  sim_base::param_entry_t{42, "TMSPWRFAN2", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSPWRFAN2), 0},                     
  sim_base::param_entry_t{43, "TMSPWRFAN3", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSPWRFAN3), 0},                     
  sim_base::param_entry_t{44, "TMSPWRFAN4", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_TMSPWRFAN4), 0},                     
  sim_base::param_entry_t{45, "TMST", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_TMST), 0}                              
};                                                                                                                                          

inline void init_defaults() {                                                                                                               
  sim_base::init_params(PARAMS);                                                                                                            
  sim_base::init_params(LOCAL_PARAMS);                                                                                                      
}                                                                                                                                           

}  // namespace sim_tms_base                                                                                                                
