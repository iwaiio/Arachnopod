file(GLOB_RECURSE SODF_SRC CONFIGURE_DEPENSODF src/*.cpp)
file(GLOB_RECURSE SODF_INC CONFIGURE_DEPENSODF include/*.hpp)
file(GLOB_RECURSE SODF_I CONFIGURE_DEPENSODF interface/*.hpp)

add_library(sodf_sim STATIC ${SODF_SRC} ${SODF_INC} ${SODF_I})
target_include_directories(sodf_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sodf_sim PUBLIC core_sim mock_sim)