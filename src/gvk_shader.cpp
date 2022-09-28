#include "gvk_shader.h"

#ifdef GVK_WINDOWS_PLATFORM
#include <Windows.h>

opt<int32> LauchProcess(const char* process,const char* options) {
	uint32 process_len = strlen(process), option_len = strlen(options);
	char buffer[1024];
	strcpy(buffer, process);
	strcat(buffer, options);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	gvk_zero_mem(si);
	gvk_zero_mem(pi);

	if (! CreateProcessA(
		NULL,   //  指向一个NULL结尾的、用来指定可执行模块的宽字节字符串  
		buffer, // 命令行字符串  
		NULL, //    指向一个SECURITY_ATTRIBUTES结构体，这个结构体决定是否返回的句柄可以被子进程继承。  
		NULL, //    如果lpProcessAttributes参数为空（NULL），那么句柄不能被继承。<同上>  
		false,//    指示新进程是否从调用进程处继承了句柄。   
		0,  //  指定附加的、用来控制优先类和进程的创建的标  
		//  CREATE_NEW_CONSOLE  新控制台打开子进程  
		//  CREATE_SUSPENDED    子进程创建后挂起，直到调用ResumeThread函数  
		NULL, //    指向一个新进程的环境块。如果此参数为空，新进程使用调用进程的环境  
		NULL, //    指定子进程的工作路径  
		&si, // 决定新进程的主窗体如何显示的STARTUPINFO结构体  
		&pi  // 接收新进程的识别信息的PROCESS_INFORMATION结构体  
	)) {
		printf("fail to create process glslc\n");
		return std::nullopt;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD value = 0;
	GetExitCodeProcess(pi.hProcess, &value);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return (int32)value;
}

#endif

#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>

namespace gvk {

	static opt<std::string> SearchUnderPathes(const char* file, const char** search_pathes, uint32 search_path_count) {
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
		if (found) return p.string();
		return std::nullopt;
	}


	opt<ptr<Shader>> Shader::Compile(const char* file, GVK_SHADER_STAGE stage,
		const ShaderMacros& macros,
		const char** include_directories, uint32 include_directory_count,
		const char** search_pathes, uint32 search_path_count, 
		std::string* error)
	{
		
		static std::string shader_stage_name_table[GVK_SHADER_STAGE_CALLABLE + 1] = {
			"vert","frag","tesc","tese","geom","comp","comp","comp","comp","comp","comp","comp","comp","comp"
		};

		std::string input;
		if (auto p = SearchUnderPathes(file,search_pathes,search_path_count)) {
			input = p.value();
		}
		else {
			if(error) *error = "gvk : input file" + std::string(file) + "  doesn't exists";
			return std::nullopt;
		}

		std::string output = input + ".spv";
		std::string options = "-fshader-stage=vert" + shader_stage_name_table[stage] + " " + input + " -o " + output;
		//add macros to glslc
		for (uint32 i = 0; i < macros.name.size();i++) {
			options += " -D" + std::string(macros.name[i]);
			if (macros.value[i] != nullptr) {
				options += "=" + std::string(macros.value[i]);
			}
		}
		//add include directories to glslc
		for (uint32 i = 0; i < include_directory_count;i++) {
			options += " -I " + std::string(include_directories[i]);
		}

		//lauch the compile process
		if (auto v = LauchProcess(GLSLC_EXECUATABLE, options.c_str()); !v.has_value())
		{
			if(error != nullptr) *error = "gvk : fail to create glslc process";
			return std::nullopt;
		}
		else if (v.value() != 0) {
			if (error != nullptr) *error = "gvk : fail to compile file " + input;
			return std::nullopt;
		}

		return LoadFromBinaryFile(output, error);
	}



	opt<ptr<Shader>> Shader::LoadFromBinaryFile(std::string& file, std::string* error)
	{
		std::ifstream binary_code_file(file, std::ios::ate | std::ios::binary);
		if (!binary_code_file.is_open()) {
			if (error != nullptr) *error = "gvk : fail to open binary file " + file;
			return std::nullopt;
		}

		uint64_t code_file_len = binary_code_file.tellg();
		char* data = (char*)malloc(code_file_len);
		binary_code_file.read(data, code_file_len);

		spv_reflect::ShaderModule shader_module(code_file_len, data, SPV_REFLECT_MODULE_FLAG_NONE);
		if (shader_module.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
		{
			if (error != nullptr) *error = "gvk : fail to create reflection for compiled shader code " + file;
			return std::nullopt;
		}

		ptr<Shader> shader(new Shader(data, code_file_len, (VkShaderStageFlagBits)shader_module.GetShaderStage()));
		shader->m_ReflectShaderModule = std::move(shader_module);
		shader->m_ShaderModule = NULL;
		return shader;
	}

	opt<ptr<Shader>> Shader::Load(const char* _file, const char** search_pathes, uint32 search_path_count, std::string* error)
	{
		std::string file = _file;
		file += ".spv";
		std::string path;
		if (auto v = SearchUnderPathes(file.c_str(), search_pathes, search_path_count); v.has_value()) {
			path = v.value();
		}
		else {
			*error = "gvk : fail to load file " + file;
			return std::nullopt;
		}

		return LoadFromBinaryFile(file, error);
	}


	Shader::~Shader() {
		free(m_ByteCode);
		if (m_ShaderModule != NULL) {
			vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
		}
	}

	VkShaderStageFlagBits Shader::GetStage()
	{
		return m_Stage;
	}

	opt<VkShaderModule> Shader::CreateShaderModule(VkDevice device) {
		//this function should not be called once
		gvk_assert(m_ShaderModule == NULL);
		
		VkShaderModuleCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.pNext = NULL;
		info.pCode = (uint32_t*)m_ByteCode;
		info.codeSize = m_ByteCodeSize;
		info.flags = 0;
		
		if (vkCreateShaderModule(device, &info, nullptr, &m_ShaderModule) == VK_SUCCESS) {
			m_Device = device;
			return m_ShaderModule;
		}
		return std::nullopt;
	}

	opt<VkShaderModule>	Shader::GetShaderModule() {
		//we should not return any null value
		if (m_ShaderModule == NULL) return std::nullopt;
		return m_ShaderModule;
	}

	template<typename T,typename Enumerator>
	opt<std::vector<T*>> GetDataFromShaderModule(const spv_reflect::ShaderModule& shader_module, Enumerator enumerator) {
		uint32 count = 0;
		if ((shader_module.*enumerator)(&count,nullptr) != SPV_REFLECT_RESULT_SUCCESS) {
			return std::nullopt;
		}
		std::vector<T*> values(count);
		gvk_assert((shader_module.*enumerator)(&count, values.data()) == SPV_REFLECT_RESULT_SUCCESS);

		return std::move(values);
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
}