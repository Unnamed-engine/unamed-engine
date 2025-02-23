# Rendering

add_library(HushRendering OBJECT
        src/WindowRenderer.cpp
        src/Vulkan/VulkanAllocatedBuffer.cpp
        src/Vulkan/VulkanRenderer.cpp
        src/Vulkan/GltfMetallicRoughness.cpp
        src/Vulkan/VulkanMeshNode.cpp
        src/Vulkan/VulkanPipelineBuilder.cpp
        src/Vulkan/VulkanLoader.cpp
        src/Vulkan/VkDescriptors.cpp
        src/Vulkan/VulkanSwapchain.cpp
        src/Vulkan/VulkanFullScreenPass.cpp
        src/Shared/Camera.cpp
        src/Shared/ShaderMaterial.cpp
        src/Shared/ImageTexture.cpp
        src/Shared/EditorCamera.cpp
        src/Shared/GltfLoadFunctions.cpp
        src/ImGui/VulkanImGuiForwarder.cpp
)

# This is the default renderer and should be switched accordingly
add_compile_definitions(HUSH_VULKAN_IMPL)

target_include_directories(HushRendering PUBLIC src)

target_link_libraries(HushRendering PUBLIC
        SDL2::SDL2
        imgui
        stb
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glm::glm
        unofficial::spirv-reflect
        fastgltf::fastgltf
        HushLog
        HushUtils
        HushInput
)

set_all_warnings(HushRendering)
