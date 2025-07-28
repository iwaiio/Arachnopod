file(GLOB_RECURSE SES_SRC CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE SES_INC CONFIGURE_DEPENDS include/*.hpp)

add_library(ses_sim STATIC ${SES_SRC} ${SES_INC})
target_include_directories(ses_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ses_sim PUBLIC core_sim mock_sim)