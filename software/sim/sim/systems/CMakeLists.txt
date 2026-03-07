file(GLOB_RECURSE ARACHNOPOD_SIM_SYSTEMS_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*/src/*/*.cpp
)

add_library(arachnopod_sim_systems STATIC
  ${ARACHNOPOD_SIM_SYSTEMS_SOURCES}
)

target_link_libraries(arachnopod_sim_systems PUBLIC
  arachnopod_sim_simulation
  arachnopod_common
)

target_include_directories(arachnopod_sim_systems PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/bus_mock/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/pss/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/tcs/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/tms/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/mns/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/ls/interface
)

target_include_directories(arachnopod_sim_systems PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/bus_mock/include
  ${CMAKE_CURRENT_SOURCE_DIR}/cs/include
  ${CMAKE_CURRENT_SOURCE_DIR}/pss/include
  ${CMAKE_CURRENT_SOURCE_DIR}/tcs/include
  ${CMAKE_CURRENT_SOURCE_DIR}/tms/include
  ${CMAKE_CURRENT_SOURCE_DIR}/mns/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ls/include
)

target_compile_features(arachnopod_sim_systems PUBLIC cxx_std_23)