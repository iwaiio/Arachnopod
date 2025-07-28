file(GLOB_RECURSE CVS_SRC CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE CVS_INC CONFIGURE_DEPENDS include/*.hpp)

add_library(cvs_sim STATIC ${CVS_SRC} ${CVS_INC})
target_include_directories(cvs_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(cvs_sim PUBLIC
    core_sim
    mock_sim
    ds_sim
    ses_sim
)