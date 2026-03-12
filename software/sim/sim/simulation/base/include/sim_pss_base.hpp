/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_pss_base {                                                                                                                    

constexpr std::size_t k_param_count = 31U;                                                                                                  

extern std::uint8_t P_PSSPWR;                                                                                                               
extern std::uint8_t P_PSSPWR12;                                                                                                             
extern std::uint8_t P_PSSPWR6;                                                                                                              
extern std::uint8_t P_PSSPWR5;                                                                                                              
extern std::uint8_t P_PSSPWR33;                                                                                                             
extern std::uint8_t P_PSSECO;                                                                                                               
extern std::int8_t P_PSSCLOCK;                                                                                                              
extern sim_base::float16_t P_PSSMAXT1;                                                                                                      
extern sim_base::float16_t P_PSSMAXT2;                                                                                                      
extern sim_base::float8_t P_PSSMAXA;                                                                                                        
extern sim_base::float8_t P_PSSMAXW;                                                                                                        
extern std::int8_t P_PSSMINC;                                                                                                               
extern sim_base::float8_t P_PSSMINV;                                                                                                        
extern sim_base::float8_t P_PSSMAXA12;                                                                                                      
extern sim_base::float8_t P_PSSMAXA6;                                                                                                       
extern sim_base::float8_t P_PSSMAXA5;                                                                                                       
extern sim_base::float8_t P_PSSMAXA33;                                                                                                      
extern sim_base::float16_t P_PSST1;                                                                                                         
extern sim_base::float16_t P_PSST2;                                                                                                         
extern sim_base::float8_t P_PSSA;                                                                                                           
extern sim_base::float8_t P_PSSW;                                                                                                           
extern sim_base::float8_t P_PSSV;                                                                                                           
extern std::int8_t P_PSSC;                                                                                                                  
extern sim_base::float8_t P_PSSA12;                                                                                                         
extern sim_base::float8_t P_PSSV12;                                                                                                         
extern sim_base::float8_t P_PSSA6;                                                                                                          
extern sim_base::float8_t P_PSSV6;                                                                                                          
extern sim_base::float8_t P_PSSA5;                                                                                                          
extern sim_base::float8_t P_PSSV5;                                                                                                          
extern sim_base::float8_t P_PSSA33;                                                                                                         
extern sim_base::float8_t P_PSSV33;                                                                                                         

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
  sim_base::param_entry_t{0, "PSSPWR", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_PSSPWR)},                              
  sim_base::param_entry_t{1, "PSSPWR12", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_PSSPWR12)},                          
  sim_base::param_entry_t{2, "PSSPWR6", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_PSSPWR6)},                            
  sim_base::param_entry_t{3, "PSSPWR5", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_PSSPWR5)},                            
  sim_base::param_entry_t{4, "PSSPWR33", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_PSSPWR33)},                          
  sim_base::param_entry_t{5, "PSSECO", 8U, false, sim_base::real_type_t::uint, static_cast<void*>(&P_PSSECO)},                              
  sim_base::param_entry_t{6, "PSSCLOCK", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_PSSCLOCK)},                           
  sim_base::param_entry_t{7, "PSSMAXT1", 16U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSMAXT1)},                        
  sim_base::param_entry_t{8, "PSSMAXT2", 16U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSMAXT2)},                        
  sim_base::param_entry_t{9, "PSSMAXA", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSMAXA)},                           
  sim_base::param_entry_t{10, "PSSMAXW", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSMAXW)},                          
  sim_base::param_entry_t{11, "PSSMINC", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_PSSMINC)},                            
  sim_base::param_entry_t{12, "PSSMINV", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSMINV)},                          
  sim_base::param_entry_t{13, "PSSMAXA12", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSMAXA12)},                      
  sim_base::param_entry_t{14, "PSSMAXA6", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSMAXA6)},                        
  sim_base::param_entry_t{15, "PSSMAXA5", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSMAXA5)},                        
  sim_base::param_entry_t{16, "PSSMAXA33", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSMAXA33)},                      
  sim_base::param_entry_t{17, "PSST1", 16U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSST1)},                             
  sim_base::param_entry_t{18, "PSST2", 16U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSST2)},                             
  sim_base::param_entry_t{19, "PSSA", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSA)},                                
  sim_base::param_entry_t{20, "PSSW", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSW)},                                
  sim_base::param_entry_t{21, "PSSV", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSV)},                                
  sim_base::param_entry_t{22, "PSSC", 8U, true, sim_base::real_type_t::sint, static_cast<void*>(&P_PSSC)},                                  
  sim_base::param_entry_t{23, "PSSA12", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSA12)},                            
  sim_base::param_entry_t{24, "PSSV12", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSV12)},                            
  sim_base::param_entry_t{25, "PSSA6", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSA6)},                              
  sim_base::param_entry_t{26, "PSSV6", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSV6)},                              
  sim_base::param_entry_t{27, "PSSA5", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSA5)},                              
  sim_base::param_entry_t{28, "PSSV5", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSV5)},                              
  sim_base::param_entry_t{29, "PSSA33", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSA33)},                            
  sim_base::param_entry_t{30, "PSSV33", 8U, true, sim_base::real_type_t::sfloat, static_cast<void*>(&P_PSSV33)}                             
};                                                                                                                                          

}  // namespace sim_pss_base                                                                                                                
