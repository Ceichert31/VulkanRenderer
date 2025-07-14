# CMakeList.txt : CMake project for VulkanRenderer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("VulkanRenderer")

# Add source to this project's executable.
add_executable (VulkanRenderer "VulkanRenderer.cpp" "VulkanRenderer.h" "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET VulkanRenderer PROPERTY CXX_STANDARD 20)
endif()

# Packages

find_package(assimp CONFIG REQUIRED)
target_link_libraries(VulkanRenderer PRIVATE assimp::assimp)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(VulkanRenderer PRIVATE glfw)

find_package(glm CONFIG REQUIRED)
target_link_libraries(VulkanRenderer PRIVATE glm::glm)

find_package(Vulkan REQUIRED)
target_link_libraries(VulkanRenderer PRIVATE Vulkan::Vulkan)

find_package(Taskflow CONFIG REQUIRED)
target_link_libraries(VulkanRenderer PRIVATE Taskflow::Taskflow)

find_package(glslang CONFIG REQUIRED)
target_link_libraries(VulkanRenderer PRIVATE glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)
