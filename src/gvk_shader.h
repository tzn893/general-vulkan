#pragma once
#include <spirv_reflect.h>
#include "gvk_common.h"


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
		static opt<ptr<Shader>> Compile(const char* file,
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

		uint32 GetDescriptorBindingCount();
		uint32 GetDescriptorSetCount();
		uint32 GetInputVariableCount();
		uint32 GetOutputVariableCount();
		uint32 GetPushConstantCount();

		const std::string& Name();

		const char* GetEntryPointName();

		~Shader();

	private:
		Shader(void* byte_code, uint64_t byte_code_size,VkShaderStageFlagBits stage,const std::string& name);

		static opt<ptr<Shader>> LoadFromBinaryFile(std::string& file,std::string* error);

		VkShaderStageFlagBits m_Stage;
		void*  m_ByteCode;
		uint64_t m_ByteCodeSize;
		spv_reflect::ShaderModule m_ReflectShaderModule;
		VkShaderModule m_ShaderModule;
		VkDevice	   m_Device;
		std::string    m_Name;
	};

}