/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "sim_base.hpp"                                                                                                                     

namespace sim_pss_base {                                                                                                                    

constexpr std::size_t k_param_count = 31U;                                                                                                  
constexpr std::size_t k_local_param_count = 0U;                                                                                             
constexpr std::size_t k_isim_param_count = 31U;                                                                                             

inline std::uint8_t P_PSSPWR = 0;                                                                                                           
inline std::uint8_t P_PSSPWR12 = 1;                                                                                                         
inline std::uint8_t P_PSSPWR6 = 1;                                                                                                          
inline std::uint8_t P_PSSPWR5 = 1;                                                                                                          
inline std::uint8_t P_PSSPWR33 = 1;                                                                                                         
inline std::uint8_t P_PSSECO = 0;                                                                                                           
inline std::int8_t P_PSSCLOCK = 0;                                                                                                          
inline float P_PSSMAXT1 = 80;                                                                                                               
inline float P_PSSMAXT2 = 80;                                                                                                               
inline float P_PSSMAXA = 12;                                                                                                                
inline float P_PSSMAXW = 290;                                                                                                               
inline std::int8_t P_PSSMINC = 5;                                                                                                           
inline float P_PSSMINV = 23;                                                                                                                
inline float P_PSSMAXA12 = 3;                                                                                                               
inline float P_PSSMAXA6 = 35;                                                                                                               
inline float P_PSSMAXA5 = 6;                                                                                                                
inline float P_PSSMAXA33 = 16.5;                                                                                                            
inline float P_PSST1 = 0;                                                                                                                   
inline float P_PSST2 = 0;                                                                                                                   
inline float P_PSSA = 0;                                                                                                                    
inline float P_PSSW = 0;                                                                                                                    
inline float P_PSSV = 0;                                                                                                                    
inline std::int8_t P_PSSC = 0;                                                                                                              
inline float P_PSSA12 = 0;                                                                                                                  
inline float P_PSSV12 = 0;                                                                                                                  
inline float P_PSSA6 = 0;                                                                                                                   
inline float P_PSSV6 = 0;                                                                                                                   
inline float P_PSSA5 = 0;                                                                                                                   
inline float P_PSSV5 = 0;                                                                                                                   
inline float P_PSSA33 = 0;                                                                                                                  
inline float P_PSSV33 = 0;                                                                                                                  

inline std::array<sim_base::param_entry_t, k_param_count> PARAMS = {                                                                        
  sim_base::param_entry_t{0, "PSSPWR", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR), 0},                           
  sim_base::param_entry_t{1, "PSSPWR12", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR12), 1},                       
  sim_base::param_entry_t{2, "PSSPWR6", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR6), 1},                         
  sim_base::param_entry_t{3, "PSSPWR5", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR5), 1},                         
  sim_base::param_entry_t{4, "PSSPWR33", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR33), 1},                       
  sim_base::param_entry_t{5, "PSSECO", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSECO), 0},                           
  sim_base::param_entry_t{6, "PSSCLOCK", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_PSSCLOCK), 0},                          
  sim_base::param_entry_t{7, "PSSMAXT1", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXT1), 80},                     
  sim_base::param_entry_t{8, "PSSMAXT2", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXT2), 80},                     
  sim_base::param_entry_t{9, "PSSMAXA", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA), 12},                        
  sim_base::param_entry_t{10, "PSSMAXW", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXW), 290},                      
  sim_base::param_entry_t{11, "PSSMINC", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_PSSMINC), 5},                           
  sim_base::param_entry_t{12, "PSSMINV", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMINV), 23},                       
  sim_base::param_entry_t{13, "PSSMAXA12", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA12), 3},                    
  sim_base::param_entry_t{14, "PSSMAXA6", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA6), 35},                     
  sim_base::param_entry_t{15, "PSSMAXA5", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA5), 6},                      
  sim_base::param_entry_t{16, "PSSMAXA33", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA33), 16.5},                 
  sim_base::param_entry_t{17, "PSST1", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSST1), 0},                           
  sim_base::param_entry_t{18, "PSST2", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSST2), 0},                           
  sim_base::param_entry_t{19, "PSSA", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA), 0},                              
  sim_base::param_entry_t{20, "PSSW", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSW), 0},                              
  sim_base::param_entry_t{21, "PSSV", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV), 0},                              
  sim_base::param_entry_t{22, "PSSC", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_PSSC), 0},                                 
  sim_base::param_entry_t{23, "PSSA12", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA12), 0},                          
  sim_base::param_entry_t{24, "PSSV12", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV12), 0},                          
  sim_base::param_entry_t{25, "PSSA6", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA6), 0},                            
  sim_base::param_entry_t{26, "PSSV6", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV6), 0},                            
  sim_base::param_entry_t{27, "PSSA5", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA5), 0},                            
  sim_base::param_entry_t{28, "PSSV5", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV5), 0},                            
  sim_base::param_entry_t{29, "PSSA33", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA33), 0},                          
  sim_base::param_entry_t{30, "PSSV33", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV33), 0}                           
};                                                                                                                                          

inline std::array<sim_base::param_entry_t, k_local_param_count> LOCAL_PARAMS = {                                                            
};                                                                                                                                          

inline std::array<sim_base::param_entry_t, k_isim_param_count> ISIM_PARAMS = {                                                              
  sim_base::param_entry_t{0, "PSSPWR", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR), 0},                           
  sim_base::param_entry_t{1, "PSSPWR12", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR12), 1},                       
  sim_base::param_entry_t{2, "PSSPWR6", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR6), 1},                         
  sim_base::param_entry_t{3, "PSSPWR5", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR5), 1},                         
  sim_base::param_entry_t{4, "PSSPWR33", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSPWR33), 1},                       
  sim_base::param_entry_t{5, "PSSECO", 1U, sim_base::physical_type_t::boolean, static_cast<void*>(&P_PSSECO), 0},                           
  sim_base::param_entry_t{6, "PSSCLOCK", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_PSSCLOCK), 0},                          
  sim_base::param_entry_t{7, "PSSMAXT1", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXT1), 80},                     
  sim_base::param_entry_t{8, "PSSMAXT2", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXT2), 80},                     
  sim_base::param_entry_t{9, "PSSMAXA", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA), 12},                        
  sim_base::param_entry_t{10, "PSSMAXW", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXW), 290},                      
  sim_base::param_entry_t{11, "PSSMINC", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_PSSMINC), 5},                           
  sim_base::param_entry_t{12, "PSSMINV", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMINV), 23},                       
  sim_base::param_entry_t{13, "PSSMAXA12", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA12), 3},                    
  sim_base::param_entry_t{14, "PSSMAXA6", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA6), 35},                     
  sim_base::param_entry_t{15, "PSSMAXA5", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA5), 6},                      
  sim_base::param_entry_t{16, "PSSMAXA33", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSMAXA33), 16.5},                 
  sim_base::param_entry_t{17, "PSST1", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSST1), 0},                           
  sim_base::param_entry_t{18, "PSST2", 16U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSST2), 0},                           
  sim_base::param_entry_t{19, "PSSA", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA), 0},                              
  sim_base::param_entry_t{20, "PSSW", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSW), 0},                              
  sim_base::param_entry_t{21, "PSSV", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV), 0},                              
  sim_base::param_entry_t{22, "PSSC", 8U, sim_base::physical_type_t::int8, static_cast<void*>(&P_PSSC), 0},                                 
  sim_base::param_entry_t{23, "PSSA12", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA12), 0},                          
  sim_base::param_entry_t{24, "PSSV12", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV12), 0},                          
  sim_base::param_entry_t{25, "PSSA6", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA6), 0},                            
  sim_base::param_entry_t{26, "PSSV6", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV6), 0},                            
  sim_base::param_entry_t{27, "PSSA5", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA5), 0},                            
  sim_base::param_entry_t{28, "PSSV5", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV5), 0},                            
  sim_base::param_entry_t{29, "PSSA33", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSA33), 0},                          
  sim_base::param_entry_t{30, "PSSV33", 8U, sim_base::physical_type_t::float32, static_cast<void*>(&P_PSSV33), 0}                           
};                                                                                                                                          

inline void init_defaults() {                                                                                                               
  sim_base::init_params(PARAMS);                                                                                                            
  sim_base::init_params(LOCAL_PARAMS);                                                                                                      
}                                                                                                                                           

}  // namespace sim_pss_base                                                                                                                
