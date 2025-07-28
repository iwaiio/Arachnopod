add_executable(arachnopod_sim main.cpp)

target_link_libraries(arachnopod_sim PRIVATE
    cvs_sim
    ds_sim
    ses_sim
    core_sim
    mock_sim
)