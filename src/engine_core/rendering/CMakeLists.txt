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
        src/Shared/Camera.cpp
        src/Shared/ImageTexture.cpp
        src/Shared/EditorCamera.cpp
        src/ImGui/VulkanImGuiForwarder.cpp
)

target_include_directories(HushRendering PUBLIC src)

target_link_libraries(HushRendering PUBLIC
        SDL2::SDL2
        imgui
        stb
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glm::glm
        fastgltf::fastgltf
        HushLog
        HushUtils
        HushInput
)

set_all_warnings(HushRendering)
