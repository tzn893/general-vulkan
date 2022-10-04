#include "gvk_shader.h"
#include <iostream>



void PrintVariable(const SpvReflectInterfaceVariable* variable) {
	std::cout << "name:" << variable->name << ", location:" << variable->location << " , format:" << variable->format << std::endl;
}


int main() {
	const char* include_dirs[] = { TEST_SHADER_DIRECTORY };
	const char* search_pathes[] = { TEST_SHADER_DIRECTORY };
	auto opt_shader = gvk::Shader::Compile(
		"frag.glsl",GVK_SHADER_STAGE_FRAGMENT,gvk::ShaderMacros(),
		include_dirs,1,search_pathes,1,nullptr);

	ptr<gvk::Shader> shader = opt_shader.value();
	auto input = shader->GetInputVariables().value();
	std::cout << "input variables:" << std::endl;
	for (auto& in : input) {
		PrintVariable(in);
	}
	std::cout << "output variables" << std::endl;
	auto output = shader->GetOutputVariables().value();
	for (auto& out : output) {
		PrintVariable(out);
	}

}