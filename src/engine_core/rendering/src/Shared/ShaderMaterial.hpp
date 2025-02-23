#pragma once
#include <filesystem>
#include <span>
#include <unordered_map>
#include "ShaderBindings.hpp"
#include "Result.hpp"
#include "Assertions.hpp"

class SpvReflectTypeDescription;

namespace Hush {
	struct OpaqueMaterialData;
#if defined(HUSH_VULKAN_IMPL)
	struct DescriptorAllocatorGrowable;
	struct VkMaterialInstance;
	class VulkanAllocatedBuffer;
	using GraphicsApiMaterialInstance = VkMaterialInstance;
	using OpaqueDescriptorAllocator = DescriptorAllocatorGrowable;
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
			PipelineLayoutCreationFailed,
			PropertyNotFound,
			ShaderNotLoaded
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

		void GenerateMaterialInstance(OpaqueDescriptorAllocator* descriptorAllocator);

		OpaqueMaterialData* GetMaterialData();

		template<class T>
		inline void SetProperty(const std::string_view& name, T value) {
			// Search for a binding with the name passed onto the func
			const ShaderBindings& binding = this->FindBinding(name);
			// Compare the binding type, and send that to the appropriate vector buffer
			// Select the vector buffer
			std::vector<std::byte>& vecBuffer = binding.type == ShaderBindings::EBindingType::UniformBufferMember
				? this->m_uniformBufferData : this->m_shaderInputData;
			// Assert that it is initialized size() > 0
			HUSH_ASSERT(vecBuffer.size() > 0, "Material buffer is not initialized! Forgot to call LoadShaders?");
			// Offset the pointer by the binding's offset
			std::byte* dataStartingPoint = vecBuffer.data() + binding.offset;
			// Memcpy the data with sizeof(T)
			memcpy(dataStartingPoint, &value, sizeof(T)); 
		}

		template<class T>
		inline Result<T, EError> GetProperty(const std::string_view& name) {
			HUSH_COND_FAIL_V(this->m_bindingsByName.find(name.data()) != this->m_bindingsByName.end(), EError::PropertyNotFound);
			// Search for a binding with the name passed onto the func
			const ShaderBindings& binding = this->FindBinding(name);

			// Compare the binding type, and send that to the appropriate vector buffer
			// Select the vector buffer
			std::vector<std::byte>& vecBuffer = binding.type == ShaderBindings::EBindingType::UniformBufferMember
				? this->m_uniformBufferData : this->m_shaderInputData;
			if (vecBuffer.size() < 1) {
				return EError::ShaderNotLoaded;
			}
			std::byte* dataStartingPoint = vecBuffer.data() + binding.offset;
			
			//Important to reinterpret cast using T, because some stuff might be 16byte-aligned and we want to only get the bytes
			//That correspond to the actual value type
			return *reinterpret_cast<T*>(dataStartingPoint);
		}

		[[nodiscard]] const GraphicsApiMaterialInstance& GetInternalMaterial() const;

	private:
		Result<std::vector<ShaderBindings>, EError> ReflectShader(std::span<std::uint32_t> shaderBinary);

		uint32_t GetAPIBinding(ShaderBindings::EBindingType agnosticBinding);

		EError BindShader(const std::vector<ShaderBindings>& vertBindings, const std::vector<ShaderBindings>& fragBindings);

		void InitializeMaterialDataMembers();

		size_t CalculateTypeSize(const SpvReflectTypeDescription* type);

		const ShaderBindings& FindBinding(const std::string_view& name);

		IRenderer* m_renderer;
		OpaqueMaterialData* m_materialData;

		// A large-ish data pool that holds all the inputs
		// for the data that needs to be sent to the material
		// womp, womp, malloc it is
		std::vector<std::byte> m_shaderInputData;

		std::vector<std::byte> m_uniformBufferData;

		std::unordered_map<std::string, ShaderBindings> m_bindingsByName;

		std::unique_ptr<GraphicsApiMaterialInstance> m_internalMaterial;
		
		void* m_uniformBufferMappedData;
	};
}
