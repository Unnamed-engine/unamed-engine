#pragma once
#include <filesystem>
#include <span>
#include "ShaderBindings.hpp"
#include "Result.hpp"

class SpvReflectTypeDescription;

namespace Hush {
	struct OpaqueMaterialData;
	struct OpaqueDescriptorAllocator;
#if defined(HUSH_VULKAN_IMPL)
	struct VkMaterialInstance;
	using GraphicsApiMaterialInstance = VkMaterialInstance;
#endif

	class IRenderer;
	/// @brief Represents a material created by a custom shader with dynamic mappings and reflections
	/// The performance impact of this class is considerable since it needs to keep track of the bindings
	/// in both RAM and GPU, as well as process the shader initially with Reflection (initialization cost)
	/// This class's interface is Rendering API agnostic
	class ShaderMaterial {
	public:
		enum class EError {
			None = 0,
			FragmentShaderNotFound,
			VertexShaderNotFound,
			ReflectionError,
			PipelineLayoutCreationFailed
		};

		enum class EShaderInputType {
			Float32,
			Vec2,
			Vec3,
			Vec4,
			Bool,
			Int
		};

		ShaderMaterial() = default;

		~ShaderMaterial();

		/// @brief Will create and bind pipelines for both shaders
		// Returns an error in case this fails (not a result because the underlying type is void)
		EError LoadShaders(IRenderer* renderer, const std::filesystem::path& fragmentShaderPath, const std::filesystem::path& vertexShaderPath);

		void GenerateMaterialInstance(OpaqueDescriptorAllocator* descriptorAllocator, void* outMaterialInstance);

	private:
		Result<std::vector<ShaderBindings>, EError> ReflectShader(std::span<std::uint32_t> shaderBinary);

		uint32_t GetAPIBinding(ShaderBindings::EBindingType agnosticBinding);

		EError BindShader(const std::vector<ShaderBindings>& vertBindings, const std::vector<ShaderBindings>& fragBindings);

		void InitializeMaterialDataMembers();

		size_t CalculateTypeSize(const SpvReflectTypeDescription* type);

		IRenderer* m_renderer;
		OpaqueMaterialData* m_materialData;

		// A large-ish data pool that holds all the inputs
		// for the data that needs to be sent to the material
		// womp, womp, malloc it is
		std::vector<std::byte> m_shaderInputData;

		std::vector<std::byte> m_uniformBufferData;

		std::unique_ptr<GraphicsApiMaterialInstance> m_internalMaterial;
	};
}