set(ARACHNOPOD_SIM_SYSTEMS_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/bus_mock/src/mock_bus.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/control/src/control_console.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/control/src/control_support.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_control_bridge.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_control_support.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_nodes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_operating_mode.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_operating_mode_support.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_scheduler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_scheduler_support.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_support.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_systems.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/cs_systems_support.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/nodes/cs_am.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/nodes/cs_cvm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/nodes/cs_llm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/nodes/cs_ls.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/nodes/cs_mns.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/nodes/cs_nm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/nodes/cs_pss.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/nodes/cs_tcs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/src/nodes/cs_tms.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ls/src/ls.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mns/src/mns.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pss/src/pss.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pss/src/pss_support.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/src/i_cmd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/src/i_cmd_exec.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/src/i_isim_registry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/src/i_msg_block.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/src/i_msg_header.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/src/i_par.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/src/i_sim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/src/i_sys.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/src/i_sys_algo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tcs/src/tcs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tms/src/tms.cpp
)

add_library(arachnopod_sim_systems STATIC
  ${ARACHNOPOD_SIM_SYSTEMS_SOURCES}
)

target_link_libraries(arachnopod_sim_systems PUBLIC
  arachnopod_sim_simulation
  arachnopod_common
)

target_include_directories(arachnopod_sim_systems PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/base
  ${CMAKE_CURRENT_SOURCE_DIR}/bus_mock/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/control/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/pss/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/tcs/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/tms/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/mns/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/ls/interface
)

target_include_directories(arachnopod_sim_systems PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/bus_mock/include
  ${CMAKE_CURRENT_SOURCE_DIR}/control/include
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/include
  ${CMAKE_CURRENT_SOURCE_DIR}/pss/include
  ${CMAKE_CURRENT_SOURCE_DIR}/sub/internal
  ${CMAKE_CURRENT_SOURCE_DIR}/tcs/include
  ${CMAKE_CURRENT_SOURCE_DIR}/tms/include
  ${CMAKE_CURRENT_SOURCE_DIR}/mns/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ls/include
)

target_compile_features(arachnopod_sim_systems PUBLIC cxx_std_23)
