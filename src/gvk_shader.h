#include <spirv_reflect.h>
#include "gvk_common.h"

enum GVK_SHADER_STAGE{
	GVK_SHADER_STAGE_VERTEX, // = VK_SHADER_STAGE_VERTEX_BIT
	GVK_SHADER_STAGE_TESSELLATION_CONTROL, // = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
	GVK_SHADER_STAGE_TESSELLATION_EVALUATION, // = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
	GVK_SHADER_STAGE_GEOMETRY, // = VK_SHADER_STAGE_GEOMETRY_BIT
	GVK_SHADER_STAGE_FRAGMENT, // = VK_SHADER_STAGE_FRAGMENT_BIT
	GVK_SHADER_STAGE_COMPUTE, // = VK_SHADER_STAGE_COMPUTE_BIT
	GVK_SHADER_STAGE_TASK, // = VK_SHADER_STAGE_TASK_BIT_NV
	GVK_SHADER_STAGE_MESH, // = VK_SHADER_STAGE_MESH_BIT_NV
	GVK_SHADER_STAGE_RAYGEN, // = VK_SHADER_STAGE_RAYGEN_BIT_KHR
	GVK_SHADER_STAGE_ANY_HIT, // = VK_SHADER_STAGE_ANY_HIT_BIT_KHR
	GVK_SHADER_STAGE_CLOSEST_HIT, // = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
	GVK_SHADER_STAGE_MISS, // = VK_SHADER_STAGE_MISS_BIT_KHR
	GVK_SHADER_STAGE_INTERSECTION, // = VK_SHADER_STAGE_INTERSECTION_BIT_KHR
	GVK_SHADER_STAGE_CALLABLE, // = VK_SHADER_STAGE_CALLABLE_BIT_KHR
};




namespace gvk {

	struct ShaderMacros {
		std::vector<const char*> name;
		std::vector<const char*> value;

		ShaderMacros& D(const char* name,const char* value = nullptr) {
			gvk_assert(name != nullptr);
			this->name.push_back(name);
			this->value.push_back(value);
			return *this;
		}
	};
	class Shader {
	public:
		static opt<ptr<Shader>> Compile(const char* file,GVK_SHADER_STAGE stage,
			const ShaderMacros& macros,
			const char** include_directories,uint32 include_directory_count,
			const char** search_pathes,uint32 search_path_count,
			std::string* error);

		static opt<ptr<Shader>> Load(const char* file,
			const char** search_pathes,uint32 search_path_count,
			std::string* error);

		VkShaderStageFlagBits GetStage();

		opt<VkShaderModule> CreateShaderModule(VkDevice device);
		opt<VkShaderModule>	GetShaderModule();

		opt<std::vector<SpvReflectDescriptorBinding*>>	GetDescriptorBindings();
		opt<std::vector<SpvReflectDescriptorSet*>>		GetDescriptorSets();
		opt<std::vector<SpvReflectInterfaceVariable*>>	GetInputVariables();
		opt<std::vector<SpvReflectInterfaceVariable*>>	GetOutputVariables();
		opt<std::vector<SpvReflectBlockVariable*>>		GetPushConstants();

		~Shader();

	private:
		Shader(void* byte_code, uint64_t byte_code_size,VkShaderStageFlagBits stage);

		static opt<ptr<Shader>> LoadFromBinaryFile(std::string& file,std::string* error);

		VkShaderStageFlagBits m_Stage;
		void*  m_ByteCode;
		uint64_t m_ByteCodeSize;
		spv_reflect::ShaderModule m_ReflectShaderModule;
		VkShaderModule m_ShaderModule;
		VkDevice	   m_Device;
	};

}