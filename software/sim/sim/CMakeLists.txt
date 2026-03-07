add_subdirectory(simulation)
add_subdirectory(systems)

add_executable(arachnopod_sim
  sim_operator.cpp
)

target_link_libraries(arachnopod_sim PRIVATE
  arachnopod_sim_systems
  arachnopod_sim_simulation
  arachnopod_common
)

target_compile_features(arachnopod_sim PRIVATE cxx_std_23)