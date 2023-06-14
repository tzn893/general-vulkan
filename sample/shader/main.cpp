#include "gvk_shader.h"
#include <iostream>



void PrintVariable(const SpvReflectInterfaceVariable* variable) 
{
	std::cout << "name:" << variable->name << ", location:" << variable->location << " , format:" << variable->format << std::endl;
}

void TestShader(const char* shader_file)
{
	const char* include_dirs[] = { TEST_SHADER_DIRECTORY };
	const char* search_pathes[] = { TEST_SHADER_DIRECTORY };

	std::cout << "======================" << shader_file << "======================\n";
	
	auto opt_shader = gvk::Shader::Compile(
		shader_file, gvk::ShaderMacros(),
		include_dirs, 1, search_pathes, 1, nullptr);

	gvk::ptr<gvk::Shader> shader = opt_shader.value();
	std::cout << "shader stage:" << shader->GetStage() << std::endl;

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
	std::cout << "descriptor sets" << std::endl;
	auto sets = shader->GetDescriptorSets().value();
	for (auto& set : sets) {
		std::cout << "\tset=" << set->set << " binding count=" << set->binding_count << std::endl;
		for (gvk::uint32 i = 0; i < set->binding_count; i++) {
			std::cout << "\t\tbinding=" << set->bindings[i]->binding << " name=" << set->bindings[i]->name << " type=" << set->bindings[i]->resource_type << std::endl;
		}
	}

	auto constants = shader->GetPushConstants().value();
	std::cout << "push constants" << std::endl;
	for (auto constant : constants)
	{
		std::cout << "\toffset=" << constant->offset << " absolute offset=" << constant->absolute_offset
			<< " size=" << constant->size << " padded size" << constant->padded_size << std::endl;
		for (gvk::uint32 i = 0; i < constant->member_count; i++) {
			auto member = constant->members[i];
			std::cout << "\t\t name:" << member.name << " offset" << member.offset << " size=" << member.size << " type description:" << member.type_description << std::endl;
		}
	}

	std::cout << "============================================\n\n\n";
}


int main() {
	const char* shaders[] = {"geom.geom","vert.vert","compute.comp", "mesh.mesh", "mesh.task"};

	for (int i = 0;i < gvk_count_of(shaders);i++)
	{
		TestShader(shaders[i]);
	}
}