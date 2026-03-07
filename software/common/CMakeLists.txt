# Common utilities (types, helpers, protocol/CRC, etc.)

file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_library(arachnopod_common STATIC ${COMMON_SOURCES})

target_include_directories(arachnopod_common PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(arachnopod_common PUBLIC cxx_std_23)
