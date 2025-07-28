file(GLOB_RECURSE DS_SRC CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE DS_INC CONFIGURE_DEPENDS include/*.hpp)

add_library(ds_sim STATIC ${DS_SRC} ${DS_INC})
target_include_directories(ds_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ds_sim PUBLIC core_sim mock_sim)