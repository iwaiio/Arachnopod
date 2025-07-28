file(GLOB_RECURSE MOCK_SRC CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE MOCK_INC CONFIGURE_DEPENDS include/*.hpp)

add_library(mock_sim STATIC ${MOCK_SRC} ${MOCK_INC})
target_include_directories(mock_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# target_link_libraries(core_sim)