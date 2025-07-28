file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE CORE_INC CONFIGURE_DEPENDS include/*.hpp)

add_library(core_sim STATIC ${CORE_SRC} ${CORE_INC})

find_package(Vulkan REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(glfw3 REQUIRED)
# find_package(glm REQUIRED)


message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan libraries: ${Vulkan_LIBRARIES}")

target_include_directories(core_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(core_sim 
        PRIVATE spdlog::spdlog
        PRIVATE fmt::fmt 
        PRIVATE Vulkan::Vulkan
        PRIVATE Qt6::Core Qt6::Gui
        PRIVATE glfw
        # PRIVATE glm::glm
)