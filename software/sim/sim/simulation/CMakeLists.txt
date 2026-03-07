file(GLOB_RECURSE ARACHNOPOD_SIM_SIMULATION_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*/src/*/*.cpp
)

add_library(arachnopod_sim_simulation STATIC
  ${ARACHNOPOD_SIM_SIMULATION_SOURCES}
)

target_link_libraries(arachnopod_sim_simulation PUBLIC
  arachnopod_common
)

target_include_directories(arachnopod_sim_simulation PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/pss/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/tcs/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/tms/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/mns/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/ls/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/inm/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/dcm/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/nm/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/cvm/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/am/interface
)

target_include_directories(arachnopod_sim_simulation PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/include
  ${CMAKE_CURRENT_SOURCE_DIR}/pss/include
  ${CMAKE_CURRENT_SOURCE_DIR}/tcs/include
  ${CMAKE_CURRENT_SOURCE_DIR}/tms/include
  ${CMAKE_CURRENT_SOURCE_DIR}/mns/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ls/include
  ${CMAKE_CURRENT_SOURCE_DIR}/inm/include
  ${CMAKE_CURRENT_SOURCE_DIR}/dcm/include
  ${CMAKE_CURRENT_SOURCE_DIR}/nm/include
  ${CMAKE_CURRENT_SOURCE_DIR}/cvm/include
  ${CMAKE_CURRENT_SOURCE_DIR}/am/include
)

target_compile_features(arachnopod_sim_simulation PUBLIC cxx_std_23)