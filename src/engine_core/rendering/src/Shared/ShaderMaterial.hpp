#pragma once
#include <filesystem>
#include <span>
#include "ShaderBindings.hpp"
#include "Result.hpp"
namespace Hush {
	struct OpaqueMaterialPipeline;
	struct OpaqueDescriptorAllocator;

	class IRenderer;
	class ShaderMaterial {
	public:
		enum class EError {
			None = 0,
			FragmentShaderNotFound,
			VertexShaderNotFound,
			ReflectionError
		};

		ShaderMaterial() = default;

		~ShaderMaterial();

		/// @brief Will create and bind pipelines for both shaders
		// Returns an error in case this fails (not a result because the underlying type is void)
		EError LoadShaders(IRenderer* renderer, const std::filesystem::path& fragmentShaderPath, const std::filesystem::path& vertexShaderPath);

		void GenerateMaterialInstance(OpaqueDescriptorAllocator* descriptorAllocator, void* outMaterialInstance);

	private:
		Result<std::vector<ShaderBindings>, EError> ReflectShader(std::span<std::uint32_t> shaderBinary);

		IRenderer* m_renderer;
		OpaqueMaterialPipeline* m_materialPipeline;
	};
}