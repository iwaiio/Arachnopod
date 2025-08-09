file(GLOB_RECURSE OS_SRC CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE OS_INC CONFIGURE_DEPENDS include/*.hpp)

add_library(os_sim STATIC ${OS_SRC} ${OS_INC})
target_include_directories(os_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(os_sim PUBLIC core_sim mock_sim)