/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include <algorithm>                                                                                                                        
#include <array>                                                                                                                            
#include <cstddef>                                                                                                                          
#include <cstdint>                                                                                                                          
#include <cmath>                                                                                                                            
#include <limits>                                                                                                                           
#include <mutex>                                                                                                                            
#include <string_view>                                                                                                                      

namespace sim_base {                                                                                                                        

// Packed real-value placeholders: C++ has no portable native 8/16-bit float.                                                               
// Storage is 8/16-bit integer; fractional values are represented via fixed-point scale helpers below.                                      
using float8_t = std::int8_t;                                                                                                               
using ufloat8_t = std::uint8_t;                                                                                                             
using float16_t = std::int16_t;                                                                                                             
using ufloat16_t = std::uint16_t;                                                                                                           

inline std::recursive_mutex& model_data_mutex() {                                                                                           
  static std::recursive_mutex mtx;                                                                                                          
  return mtx;                                                                                                                               
}                                                                                                                                           

template <typename PackedT>                                                                                                                 
inline PackedT pack_fixed(const float value, float scale = 1.0F) {                                                                          
  if (!(scale > 0.0F)) {                                                                                                                    
    scale = 1.0F;                                                                                                                           
  }                                                                                                                                         
                                                                                                                                            
  const float scaled = value * scale;                                                                                                       
  const auto rounded = static_cast<long>(std::lround(scaled));                                                                              
  const auto lo = static_cast<long>(std::numeric_limits<PackedT>::lowest());                                                                
  const auto hi = static_cast<long>(std::numeric_limits<PackedT>::max());                                                                   
  const auto clamped = std::clamp(rounded, lo, hi);                                                                                         
  return static_cast<PackedT>(clamped);                                                                                                     
}                                                                                                                                           
                                                                                                                                            
template <typename PackedT>                                                                                                                 
inline float unpack_fixed(const PackedT raw, float scale = 1.0F) {                                                                          
  if (!(scale > 0.0F)) {                                                                                                                    
    scale = 1.0F;                                                                                                                           
  }                                                                                                                                         
  return static_cast<float>(raw) / scale;                                                                                                   
}                                                                                                                                           
                                                                                                                                            
inline float8_t pack_float8(const float value, const float scale = 1.0F) {                                                                  
  return pack_fixed<float8_t>(value, scale);                                                                                                
}                                                                                                                                           
                                                                                                                                            
inline ufloat8_t pack_ufloat8(const float value, const float scale = 1.0F) {                                                                
  return pack_fixed<ufloat8_t>(std::max(0.0F, value), scale);                                                                               
}                                                                                                                                           
                                                                                                                                            
inline float16_t pack_float16(const float value, const float scale = 1.0F) {                                                                
  return pack_fixed<float16_t>(value, scale);                                                                                               
}                                                                                                                                           
                                                                                                                                            
inline ufloat16_t pack_ufloat16(const float value, const float scale = 1.0F) {                                                              
  return pack_fixed<ufloat16_t>(std::max(0.0F, value), scale);                                                                              
}                                                                                                                                           
                                                                                                                                            
inline float unpack_float8(const float8_t raw, const float scale = 1.0F) {                                                                  
  return unpack_fixed(raw, scale);                                                                                                          
}                                                                                                                                           
                                                                                                                                            
inline float unpack_ufloat8(const ufloat8_t raw, const float scale = 1.0F) {                                                                
  return unpack_fixed(raw, scale);                                                                                                          
}                                                                                                                                           
                                                                                                                                            
inline float unpack_float16(const float16_t raw, const float scale = 1.0F) {                                                                
  return unpack_fixed(raw, scale);                                                                                                          
}                                                                                                                                           
                                                                                                                                            
inline float unpack_ufloat16(const ufloat16_t raw, const float scale = 1.0F) {                                                              
  return unpack_fixed(raw, scale);                                                                                                          
}                                                                                                                                           
                                                                                                                                            
enum class real_type_t : std::uint8_t {                                                                                                     
  sint = 0,                                                                                                                                 
  uint = 1,                                                                                                                                 
  sfloat = 2,                                                                                                                               
  ufloat = 3,                                                                                                                               
};                                                                                                                                          

struct param_entry_t {                                                                                                                      
  std::uint16_t id;                                                                                                                         
  const char* key;                                                                                                                          
  std::uint8_t bits;                                                                                                                        
  bool is_signed;                                                                                                                           
  real_type_t type_r;                                                                                                                       
  void* ptr;                                                                                                                                
};                                                                                                                                          

inline void zero_param(param_entry_t& entry) {
  if (entry.ptr == nullptr) {
    return;
  }

  if (entry.bits <= 8U) {
    if (entry.type_r == real_type_t::sfloat) {
      *static_cast<float8_t*>(entry.ptr) = 0;
    } else if (entry.type_r == real_type_t::ufloat) {
      *static_cast<ufloat8_t*>(entry.ptr) = 0;
    } else if (entry.is_signed) {
      *static_cast<std::int8_t*>(entry.ptr) = 0;
    } else {
      *static_cast<std::uint8_t*>(entry.ptr) = 0;
    }
    return;
  }

  if (entry.type_r == real_type_t::sfloat) {
    *static_cast<float16_t*>(entry.ptr) = 0;
  } else if (entry.type_r == real_type_t::ufloat) {
    *static_cast<ufloat16_t*>(entry.ptr) = 0;
  } else if (entry.is_signed) {
    *static_cast<std::int16_t*>(entry.ptr) = 0;
  } else {
    *static_cast<std::uint16_t*>(entry.ptr) = 0;
  }
}

template <std::size_t N>
inline void zero_params(std::array<param_entry_t, N>& params) {
  for (auto& entry : params) {
    zero_param(entry);
  }
}

template <std::size_t N>                                                                                                                    
inline param_entry_t* find_param_by_id(std::array<param_entry_t, N>& params, const std::uint16_t id) {                                      
  for (auto& entry : params) {                                                                                                              
    if (entry.id == id) {                                                                                                                   
      return &entry;                                                                                                                        
    }                                                                                                                                       
  }                                                                                                                                         
  return nullptr;                                                                                                                           
}                                                                                                                                           

template <std::size_t N>                                                                                                                    
inline const param_entry_t* find_param_by_id(const std::array<param_entry_t, N>& params, const std::uint16_t id) {                          
  for (const auto& entry : params) {                                                                                                        
    if (entry.id == id) {                                                                                                                   
      return &entry;                                                                                                                        
    }                                                                                                                                       
  }                                                                                                                                         
  return nullptr;                                                                                                                           
}                                                                                                                                           

template <std::size_t N>                                                                                                                    
inline param_entry_t* find_param_by_key(std::array<param_entry_t, N>& params, const std::string_view key) {                                 
  for (auto& entry : params) {                                                                                                              
    if (key == entry.key) {                                                                                                                 
      return &entry;                                                                                                                        
    }                                                                                                                                       
  }                                                                                                                                         
  return nullptr;                                                                                                                           
}                                                                                                                                           

template <std::size_t N>                                                                                                                    
inline const param_entry_t* find_param_by_key(const std::array<param_entry_t, N>& params, const std::string_view key) {                     
  for (const auto& entry : params) {                                                                                                        
    if (key == entry.key) {                                                                                                                 
      return &entry;                                                                                                                        
    }                                                                                                                                       
  }                                                                                                                                         
  return nullptr;                                                                                                                           
}                                                                                                                                           

}  // namespace sim_base                                                                                                                    
