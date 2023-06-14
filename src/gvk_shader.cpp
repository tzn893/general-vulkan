#include "gvk_shader.h"

#ifdef GVK_WINDOWS_PLATFORM
#include <Windows.h>

gvk::opt<int32_t> LauchProcess(const char* process,const char* options) {
	gvk::uint32 process_len = strlen(process), option_len = strlen(options);
	char buffer[1024];
	strcpy(buffer, process);
	strcat(buffer, options);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	gvk_zero_mem(si);
	gvk_zero_mem(pi);

	if (!CreateProcessA(NULL,buffer,NULL,NULL,false,0,NULL,NULL,&si, &pi)) 
	{
		printf("fail to create process glslc\n");
		return std::nullopt;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD value = 0;
	GetExitCodeProcess(pi.hProcess, &value);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return (int32_t)value;
}

#endif

#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>

namespace gvk {

	Shader::Shader(void* byte_code, uint64_t byte_code_size, VkShaderStageFlagBits stage, const std::string& name) :m_ByteCode(byte_code),
		m_ByteCodeSize(byte_code_size), m_Stage(stage), m_Device(NULL), m_ShaderModule(NULL), m_Name(name) {}

	static opt<fs::path> SearchUnderPathes(const char* file, const char** search_pathes, uint32 search_path_count) {
		fs::path p(file);
		bool found = true;
		//if the file path as relative path doesn't exists
		if (!fs::exists(p))
		{
			found = false;
			//search the file in search pathes
			for (uint32 i = 0; i < search_path_count; i++)
			{
				fs::path dir_path = fs::path(search_pathes[i]) / p;
				if (fs::exists(dir_path))
				{
					p = dir_path;
					found = true;
					break;
				}
			}
		}
		if (found) return p;
		return std::nullopt;
	}


	struct ShaderCompileOptions
	{
		std::vector<std::string> include_directories;
		std::vector<std::string> macros;
		std::vector<std::string> definitions;
		std::string stage;
		std::string file;
		std::string output_file;
		std::string target_env;
		std::string target_spv;
	};


	std::string GenerateCommandLine(ShaderCompileOptions& options)
	{
		std::string cmd;
		for (uint32 i = 0; i < options.macros.size(); i++)
		{
			cmd += std::string(" -D") + std::string(options.macros[i]);
			if (options.definitions[i] != "")
			{
				cmd += "=" + std::string(options.definitions[i]);
			}
		}

		if (options.target_env != "")
		{
			cmd += "- -target-env=vulkan" + options.target_env;
		}
		if (options.target_spv != "")
		{
			cmd += " --target-spv=spv" + options.target_spv;
		}

		for (uint32 i = 0; i < options.include_directories.size(); i++)
		{
			cmd += " -I " + options.include_directories[i];
		}

		gvk_assert(options.stage != "");
		gvk_assert(options.file != "");
		cmd += " " + options.file + " -o " + options.output_file;
		return cmd;
	}


	opt<ptr<gvk::Shader>> Shader::Compile(const char* file,
		const ShaderMacros& macros,
		const char** include_directories, uint32 include_directory_count,
		const char** search_pathes, uint32 search_path_count,
		std::string* error)
	{

		std::string input, ext;
		if (auto p = SearchUnderPathes(file, search_pathes, search_path_count))
		{
			input = p.value().string();
			ext = p.value().extension().string();
		}
		else
		{
			if (error) *error = "gvk : input file" + std::string(file) + "  doesn't exists";
			return std::nullopt;
		}
		
		ShaderCompileOptions options;
		options.stage = ext.substr(1, ext.size() - 1);
		if (options.stage == "rhit" || ext == "rgen" || ext == "rmiss")
		{
			options.target_env = "1.2";
		}
		if (options.stage == "mesh" || options.stage == "task")
		{
			options.target_spv = "1.4";
		}
		if (options.stage == "vert")
		{
			options.macros.push_back("VERTEX_SHADER");
			options.definitions.push_back("");
		}
		if (options.stage == "geom")
		{
			options.macros.push_back("GEOMETRY_SHADER");
			options.definitions.push_back("");
		}
		if (options.stage == "frag")
		{
			options.macros.push_back("FRAGMENT_SHADER");
			options.definitions.push_back("");
		}

		for (uint32 i = 0; i < include_directory_count; i++) 
		{
			options.include_directories.push_back(std::string(include_directories[i]));
		}
		options.include_directories.push_back(GVK_SHADER_COMMON_DIRECTORY);
		
		options.file = input;
		options.output_file = input + ".spv";

		for (uint32 i = 0; i < macros.name.size(); i++)
		{
			options.macros.push_back(macros.name[i]);
			options.definitions.push_back((macros.value[i] == nullptr ? "" : macros.value[i]));
		}

		//lauch the compile process
		if (auto v = LauchProcess(GLSLC_EXECUATABLE, GenerateCommandLine(options).c_str()); !v.has_value())
		{
			if (error != nullptr) *error = "gvk : fail to create glslc process";
			return std::nullopt;
		}
		else if (v.value() != 0) {
			if (error != nullptr) *error = "gvk : fail to compile file " + input;
			return std::nullopt;
		}

		auto opt_shader =  LoadFromBinaryFile(options.output_file, error);

		// unfortunately reflection can't read mesh shader stage
		// we have to set it manually
		if (options.stage == "mesh" && opt_shader.has_value())
		{
			opt_shader.value()->m_Stage = VK_SHADER_STAGE_MESH_BIT_EXT;
		}
		if (options.stage == "task" && opt_shader.has_value())
		{
			opt_shader.value()->m_Stage = VK_SHADER_STAGE_TASK_BIT_EXT;
		}

		return opt_shader;
	}



	opt<ptr<Shader>> Shader::LoadFromBinaryFile(std::string& file, std::string* error)
	{
		std::ifstream binary_code_file(file, std::ios::ate | std::ios::binary);
		if (!binary_code_file.is_open()) {
			if (error != nullptr) *error = "gvk : fail to open binary file " + file;
			return std::nullopt;
		}

		uint64_t code_file_len = binary_code_file.tellg();
		binary_code_file.seekg(0, std::ios::beg);
		char* data = (char*)malloc(code_file_len);
		binary_code_file.read(data, code_file_len);

		spv_reflect::ShaderModule shader_module(code_file_len, data, SPV_REFLECT_MODULE_FLAG_NONE);
		if (shader_module.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
		{
			if (error != nullptr) *error = "gvk : fail to create reflection for compiled shader code " + file;
			return std::nullopt;
		}

		ptr<Shader> shader(new Shader(data, code_file_len, (VkShaderStageFlagBits)shader_module.GetShaderStage(), fs::path(file).filename().string()));
		shader->m_ReflectShaderModule = std::move(shader_module);
		shader->m_ShaderModule = NULL;
		return shader;
	}

	opt<ptr<Shader>> Shader::Load(const char* _file, const char** search_pathes, uint32 search_path_count, std::string* error)
	{
		std::string file = _file;
		file += ".spv";
		std::string path;
		if (auto v = SearchUnderPathes(file.c_str(), search_pathes, search_path_count); v.has_value()) 
		{
			path = v.value().string();
		}
		else 
		{
			*error = "gvk : fail to load file " + file;
			return std::nullopt;
		}

		return LoadFromBinaryFile(file, error);
	}


	Shader::~Shader() 
	{
		free(m_ByteCode);
		if (m_ShaderModule != NULL) 
		{
			vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
		}
	}

	VkShaderStageFlagBits Shader::GetStage()
	{
		return m_Stage;
	}

	opt<VkShaderModule> Shader::CreateShaderModule(VkDevice device) 
	{
		//this function should not be called once
		gvk_assert(m_ShaderModule == NULL);

		VkShaderModuleCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.pNext = NULL;
		info.pCode = (uint32_t*)m_ByteCode;
		info.codeSize = m_ByteCodeSize;
		info.flags = 0;

		if (vkCreateShaderModule(device, &info, nullptr, &m_ShaderModule) == VK_SUCCESS) 
		{
			m_Device = device;
			return m_ShaderModule;
		}
		return std::nullopt;
	}

	opt<VkShaderModule>	Shader::GetShaderModule() 
	{
		//we should not return any null value
		if (m_ShaderModule == NULL) return std::nullopt;
		return m_ShaderModule;
	}

	template<typename T, typename Enumerator>
	opt<std::vector<T*>> GetDataFromShaderModule(const spv_reflect::ShaderModule& shader_module, Enumerator enumerator) 
	{
		uint32 count = 0;
		if ((shader_module.*enumerator)(&count, nullptr) != SPV_REFLECT_RESULT_SUCCESS) 
		{
			return std::nullopt;
		}
		std::vector<T*> values(count);
		gvk_assert((shader_module.*enumerator)(&count, values.data()) == SPV_REFLECT_RESULT_SUCCESS);

		return std::move(values);
	}

	template<typename Enumerator>
	uint32 GetDataElementCountFromShaderModule(const spv_reflect::ShaderModule& shader_module,Enumerator enumerator) 
	{
		uint32 count = 0;
		(shader_module.*enumerator)(&count, nullptr);
		return count;
	}

	opt<std::vector<SpvReflectDescriptorBinding*>> Shader::GetDescriptorBindings()
	{
		return GetDataFromShaderModule<SpvReflectDescriptorBinding>
			(m_ReflectShaderModule, &spv_reflect::ShaderModule::EnumerateDescriptorBindings);
	}

	opt<std::vector<SpvReflectDescriptorSet*>> Shader::GetDescriptorSets()
	{
		return GetDataFromShaderModule<SpvReflectDescriptorSet>
			(m_ReflectShaderModule, &spv_reflect::ShaderModule::EnumerateDescriptorSets);
	}

	opt<std::vector<SpvReflectInterfaceVariable*>> Shader::GetInputVariables()
	{
		return GetDataFromShaderModule<SpvReflectInterfaceVariable>
			(m_ReflectShaderModule, &spv_reflect::ShaderModule::EnumerateInputVariables);
	}

	opt<std::vector<SpvReflectInterfaceVariable*>> Shader::GetOutputVariables()
	{
		return GetDataFromShaderModule<SpvReflectInterfaceVariable>
			(m_ReflectShaderModule, &spv_reflect::ShaderModule::EnumerateOutputVariables);
	}


	opt<std::vector<SpvReflectBlockVariable*>>	Shader::GetPushConstants() 
	{
		return GetDataFromShaderModule<SpvReflectBlockVariable>
			(m_ReflectShaderModule, &spv_reflect::ShaderModule::EnumeratePushConstantBlocks);
	}

	uint32 Shader::GetDescriptorBindingCount() 
	{
		return GetDataElementCountFromShaderModule(m_ReflectShaderModule,
			&spv_reflect::ShaderModule::EnumerateDescriptorBindings);
	}
	uint32 Shader::GetDescriptorSetCount() 
	{
		return GetDataElementCountFromShaderModule(m_ReflectShaderModule,
			&spv_reflect::ShaderModule::EnumerateDescriptorSets);
	}
	uint32 Shader::GetInputVariableCount() 
	{
		return GetDataElementCountFromShaderModule(m_ReflectShaderModule,
			&spv_reflect::ShaderModule::EnumerateInputVariables);
	}
	uint32 Shader::GetOutputVariableCount() 
	{
		return GetDataElementCountFromShaderModule(m_ReflectShaderModule,
			&spv_reflect::ShaderModule::EnumerateOutputVariables);
	}
	uint32 Shader::GetPushConstantCount() 
	{
		return GetDataElementCountFromShaderModule(m_ReflectShaderModule,
			&spv_reflect::ShaderModule::EnumeratePushConstants);
	}

	const std::string& Shader::Name()
	{
		return m_Name;
	}

	const char* Shader::GetEntryPointName()
	{
		return m_ReflectShaderModule.GetEntryPointName();
	}

}