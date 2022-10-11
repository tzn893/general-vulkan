#include "gvk_pipeline.h"
#include "gvk_context.h"

namespace gvk {
	
	DescriptorSetLayout::DescriptorSetLayout(VkDescriptorSetLayout layout, const std::vector<ptr<gvk::Shader>>& shaders,
		const std::vector<SpvReflectDescriptorBinding*>& descriptor_set_bindings, uint32 sets,VkDevice device):
	m_Shader(shaders),m_DescriptorSetBindings(descriptor_set_bindings),m_Set(sets),m_Layout(layout) ,m_Device(device)
	{}

	uint32 DescriptorSetLayout::GetSetID()
	{
		return m_Set;
	}

	VkDescriptorSetLayout DescriptorSetLayout::GetLayout()
	{
		return m_Layout;
	}

	View<SpvReflectDescriptorBinding*> DescriptorSetLayout::GetDescriptorSetBindings() 
	{
		return View<SpvReflectDescriptorBinding*>(m_DescriptorSetBindings);
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
	}

	bool DescriptorSetLayout::CreatedFromShader(const ptr<gvk::Shader>& _shader, uint32 set)
	{
		for (auto shader : m_Shader) {
			if (shader == _shader) return set == m_Set;
		}
		return false;
	}

	opt<ptr<DescriptorSetLayout>> Context::CreateDescriptorSetLayout(const std::vector<ptr<Shader>>& target_shaders, uint32 target_set,std::string* error)
	{
		std::vector<SpvReflectDescriptorBinding*> bindings;
		std::vector<VkShaderStageFlags> binding_stage_flags;
		for (auto& shader : target_shaders) {
			SpvReflectDescriptorSet* matching_set;
			if (auto v = shader->GetDescriptorSets(); v.has_value())
			{
				auto sets = std::move(v.value());
				auto find_res = std::find_if(sets.begin(), sets.end(), 
					[&](SpvReflectDescriptorSet* set) {
						return set->set == target_set;
					}
				);
				if (find_res == sets.end()) 
				{
					if (error) 
					{
						*error = "gvk : fail to create descriptor binding layout, in shader " + shader->Name() +
							" set " + std::to_string(target_set) + " doesn't exists";
					}
					return std::nullopt;
				}
				matching_set = *find_res;
			}
			else 
			{
				if(error) *error = "gvk : fail to get descriptor set from shader " + shader->Name();
				return std::nullopt;
			}
			
			for (uint32 i = 0; i < matching_set->binding_count;i++) 
			{
				SpvReflectDescriptorBinding* binding = matching_set->bindings[i];
				auto res = std::find_if(bindings.begin(), bindings.end(),
					[&](SpvReflectDescriptorBinding* set_binding) 
					{
						return binding->binding == set_binding->binding;
					});
				if (res == bindings.end()) 
				{
					bindings.push_back(binding);
					binding_stage_flags.push_back(shader->GetStage());
				}
				//descriptor at the same binding place should have the same name, descriptor type,
				//descriptor format
				else 
				{
					SpvReflectDescriptorBinding* set_binding = *res;
					//check if the shader's binding is compatiable with existing bindings
					if (set_binding->descriptor_type != binding->descriptor_type) 
					{
						if (error) *error = "gvk : fail to create descriptor layout at (set " + std::to_string(target_set) +  
							", binding " + std::to_string(binding->binding) + ") shader " + shader->Name() + "'s descriptor type "
							"doesn't match other shader's binding";
						return std::nullopt;
					}
					if (set_binding->resource_type != binding->resource_type) 
					{
						if (error) *error = "gvk : fail to create descriptor layout at (set " + std::to_string(target_set) +
							", binding " + std::to_string(binding->binding) + ") shader " + shader->Name() + "'s resource type "
							"doesn't match other shader's resource type";
						return std::nullopt;
					}
					if (set_binding->count != binding->count) {
						if (error) *error = "gvk : fail to create descriptor layout at (set " + std::to_string(target_set) +
							", binding " + std::to_string(binding->binding) + ") shader " + shader->Name() + "'s count " + std::to_string(binding->count)
							+ "doesn't match other shader's resource type";
						return std::nullopt;
					}
					
					bool array_dims_equal = set_binding->array.dims_count == binding->array.dims_count;
					for (uint32 j = 0; j < set_binding->array.dims_count && array_dims_equal;j++) 
					{
						array_dims_equal = set_binding->array.dims[j] == binding->array.dims[j];
					}
					if (!array_dims_equal) 
					{
						if (error) *error = "gvk : fail to create descriptor layout at (set " + std::to_string(target_set) +
							", binding " + std::to_string(binding->binding) + ") shader " + shader->Name() + "'s array dimension "
							"doesn't match other shader's resource type";
						return std::nullopt;
					}

					bool image_dims_equal = memcmp(&set_binding->image,&binding->image,sizeof(set_binding->image)) == 0;
					if (!image_dims_equal) {
						if (error) *error = "gvk : fail to create descriptor layout at (set " + std::to_string(target_set) +
							", binding " + std::to_string(binding->binding) + ") shader " + shader->Name() + "'s array dimension "
							"doesn't match other shader's resource type";
						return std::nullopt;
					}

					//check passed 
					binding_stage_flags[res - bindings.begin()] |= shader->GetStage();
				}
			}
		}
		

		VkDescriptorSetLayoutCreateInfo info{};
		info.bindingCount = bindings.size();
		info.flags = 0;
		std::vector<VkDescriptorSetLayoutBinding> vk_bindings(bindings.size());
		for (uint32 i = 0; i < bindings.size(); i++)
		{
			vk_bindings[i].binding = bindings[i]->binding;
			//they are equal
			vk_bindings[i].descriptorType = (VkDescriptorType)bindings[i]->descriptor_type;
			//TODO : currently we don't support immutable samplers
			vk_bindings[i].pImmutableSamplers = NULL;
			vk_bindings[i].stageFlags = binding_stage_flags[i];
		}
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pNext = NULL;
		info.pBindings = vk_bindings.data();
		VkDescriptorSetLayout layout;
		VkResult rs = vkCreateDescriptorSetLayout(m_Device, &info, nullptr, &layout);
		if (rs != VK_SUCCESS)
		{
			if (error) *error = "gvk : fail to create descriptor set layout";
			return std::nullopt;
		}

		return ptr<DescriptorSetLayout>(new DescriptorSetLayout(layout, target_shaders, bindings, target_set,m_Device));	
	}


	opt<ptr<GraphicsPipeline>> Context::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& info) {
		VkGraphicsPipelineCreateInfo vk_create_info{};
		vk_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		vk_create_info.pNext = NULL;
		vk_create_info.pColorBlendState = &info.frame_buffer_blend_state.create_info;

		if (info.depth_stencil_state.enable_depth_stencil) 
		{
			vk_create_info.pDepthStencilState = &info.depth_stencil_state;
		}
		else 
		{
			//if depth stencil state is not enabled
			//pass null pointer to pDepthStencilState
			vk_create_info.pDepthStencilState = NULL;
		}

		//By default we will set scissor and viewport as dynamic state
		VkDynamicState dynamic_states[] = {
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_VIEWPORT
		};
		VkPipelineDynamicStateCreateInfo dynamic_state_info{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
		dynamic_state_info.dynamicStateCount = gvk_count_of(dynamic_states);
		dynamic_state_info.pDynamicStates = dynamic_states;
		
		vk_create_info.pDynamicState = &dynamic_state_info;
		vk_create_info.pMultisampleState = &info.multi_sample_state;
		vk_create_info.pRasterizationState = &info.rasterization_state;
		vk_create_info.pInputAssemblyState = &info.input_assembly_state;
	
		

		//input vertex attributes
		//TODO : currently we don't support multiple vertex bindings
		VkPipelineVertexInputStateCreateInfo vertex_input_state{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertex_input_state.flags = 0;

		std::vector<SpvReflectInterfaceVariable*> vertex_input;
		if (auto v = info.vertex_shader->GetInputVariables(); v.has_value())
		{
			vertex_input = std::move(v.value());
		}
		else
		{
			return std::nullopt;
		}

		std::vector<VkVertexInputAttributeDescription> attributes(vertex_input.size());
		uint32 total_stride = 0, current_offset = 0;
		for (uint32 i = 0;i < vertex_input.size();i++) 
		{
			auto member = vertex_input[i];
			uint32 size = GetFormatSize((VkFormat)member->format);
			total_stride += size;

			//TODO: currently we only support 1 binding
			attributes[i].binding = 0;
			attributes[i].format = (VkFormat)member->format;
			attributes[i].location = member->location;
			attributes[i].offset = current_offset;

			current_offset += size;
		}

		VkVertexInputBindingDescription binding{};
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		binding.binding = 0;
		binding.stride = total_stride;

		vertex_input_state.pVertexAttributeDescriptions = attributes.data();
		vertex_input_state.vertexAttributeDescriptionCount = attributes.size();

		//TODO : currently we only support 1 binding
		vertex_input_state.pVertexBindingDescriptions = &binding;
		vertex_input_state.vertexBindingDescriptionCount = 1;

		vk_create_info.pVertexInputState = &vertex_input_state;
		

		//shader stages
		std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
		
		auto create_shader_stage = [&](ptr<Shader> shader) 
		{
			gvk_assert(shader != nullptr);
			VkPipelineShaderStageCreateInfo shader_stage_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
			//currently we ignore shader stage flags
			//for details see https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#interfaces-builtin-variables-sgs
			shader_stage_info.flags = 0;
			if (auto v = shader->GetShaderModule();v.has_value()) 
			{
				shader_stage_info.module = v.value();
			}
			else 
			{
				return false;
			}

			shader_stage_info.pName = shader->Name().c_str();
			//we don't define any specialization
			//for details see https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VkSpecializationInfo
			shader_stage_info.pSpecializationInfo = NULL;
			shader_stage_info.stage = shader->GetStage();
			
			shader_stage_infos.push_back(shader_stage_info);
		};

		if (!create_shader_stage(info.vertex_shader)) 
		{
			return std::nullopt;
		}
		if (info.fragment_shader != nullptr && !create_shader_stage(info.fragment_shader)) 
		{
			return std::nullopt;
		}

		vk_create_info.pStages = shader_stage_infos.data();
		vk_create_info.stageCount = shader_stage_infos.size();

		// pipeline layouts
		//We create internal descriptor sets for every shader 
		VkPipelineLayoutCreateInfo pipeline_layout_create_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		std::vector<VkDescriptorSetLayout> descriptor_layouts;
		//a bit map check whether the precluded descriptor layouts is included to descriptor layouts for pipeline creation
		std::vector<bool> layout_included(info.descriptor_layuot_hint.precluded_descriptor_layouts.size(), false);
		std::vector<ptr<DescriptorSetLayout>> internal_layout;

		//Push constant information
		std::vector<VkPushConstantRange> push_constant_ranges;
		std::unordered_map<std::string, VkPushConstantRange> push_constant_table;
		auto collect_desc_layout_for_shader = [&](ptr<Shader> shader) {
			if (shader == nullptr) return false;

			std::vector<SpvReflectDescriptorSet*> sets;
			if (auto v = shader->GetDescriptorSets(); v.has_value())
			{
				sets = std::move(v.value());
			}
			else
			{
				return false;
			}

			//collect descriptor set layout information
			for (auto set : sets)
			{
				bool is_layout_precluded = false;
				for (uint32 i = 0; i < layout_included.size(); i++)
				{
					auto layout = info.descriptor_layuot_hint.precluded_descriptor_layouts[i];
					//does this layout is created from this shader
					if (layout->CreatedFromShader(shader, set->set))
					{
						//this layout is not included in layout list
						if (!layout_included[i])
						{
							descriptor_layouts.push_back(layout->GetLayout());
							layout_included[i] = true;
							is_layout_precluded = true;
							break;
						}
					}
				}
				if (!is_layout_precluded)
				{
					//this layout is not included in layout list create a new layout for this set
					auto opt_layout = CreateDescriptorSetLayout({ shader }, set->set, nullptr);
					//this operation must success or something wrong with my code
					gvk_assert(opt_layout.has_value());
					internal_layout.push_back(opt_layout.value());
					descriptor_layouts.push_back(opt_layout.value()->GetLayout());
				}
			}

			//collect push constant information
			std::vector<SpvReflectBlockVariable*> push_constants;
			if (auto v = shader->GetPushConstants(); v.has_value())
			{
				push_constants = std::move(v.value());
			}
			else
			{
				return false;
			}

			//if the shader has push constant
			if (!push_constants.empty())
			{
				//in glsl only one push_constant block is supported
				auto push_constant = push_constants[0];

				for (uint32 i = 0; i < push_constant->member_count; i++)
				{
					auto member = push_constant->members[i];
					if (!push_constant_table.count(member.name))
					{
						push_constant_table[member.name] = { VkPushConstantRange{ (VkShaderStageFlags)shader->GetStage(),member.offset,member.size } };
					}
					else
					{
						auto& push_constant = push_constant_table[member.name];
						//variables with the same name in different push constants should be consistent to each other
						if (member.offset == push_constant.offset && member.size == push_constant.size)
						{
							push_constant.stageFlags |= shader->GetStage();
						}
						else
						{
							//variables conflict, operation fails
							return false;
						}
					}
				}

				VkPushConstantRange push_constant_range;
				push_constant_range.offset = push_constant->offset;
				push_constant_range.size = push_constant->size;
				push_constant_range.stageFlags = shader->GetStage();

				push_constant_ranges.push_back(push_constant_range);
			}


			return true;
		};

		if (!collect_desc_layout_for_shader(info.vertex_shader))
		{
			return std::nullopt;
		}
		if (!collect_desc_layout_for_shader(info.fragment_shader)) {
			return std::nullopt;
		}

		pipeline_layout_create_info.flags = 0;
		pipeline_layout_create_info.pSetLayouts = descriptor_layouts.data();
		pipeline_layout_create_info.setLayoutCount = descriptor_layouts.size();
		pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();
		pipeline_layout_create_info.pushConstantRangeCount = push_constant_ranges.size();

		VkPipelineLayout pipeline_layout;
		if (vkCreatePipelineLayout(m_Device, &pipeline_layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
		{
			return std::nullopt;
		}

		vk_create_info.layout = pipeline_layout;

		return std::nullopt;
	}


}

GraphicsPipelineCreateInfo::FrameBufferBlendState::FrameBufferBlendState()
{
	create_info.pNext = NULL;
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	create_info.attachmentCount = 0;
	//ignore the flags
	create_info.flags = 0;
	//currently we don't support logic operation
	create_info.logicOpEnable = VK_FALSE;
	create_info.pAttachments = NULL;
}

void GraphicsPipelineCreateInfo::FrameBufferBlendState::Resize(uint32 frame_buffer_count)
{
	//create a default blend state for every resized states
	VkPipelineColorBlendAttachmentState default_blend_state = BlendState();
	frame_buffer_states.resize(frame_buffer_count, default_blend_state);

	create_info.attachmentCount = frame_buffer_count;
	create_info.pAttachments = frame_buffer_states.data();
}

void GraphicsPipelineCreateInfo::FrameBufferBlendState::Set(uint32 index, const BlendState& state)
{
	gvk_assert(index < frame_buffer_states.size());
	frame_buffer_states[index] = state;
}


GraphicsPipelineCreateInfo::BlendState::BlendState()
{
	//by default blend is disabled
	blendEnable = VK_FALSE;
	//by default all write mask is no
	colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
	alphaBlendOp = VK_BLEND_OP_ADD;
	srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	// color = src_color * src_alpha + ( 1 - src_alpha ) * dst_color
	colorBlendOp = VK_BLEND_OP_ADD;
	srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
}

GraphicsPipelineCreateInfo::InputAssembly::InputAssembly()
{
	sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pNext = NULL;
	flags = 0;
	// most primitive we draw are triangle list
	topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// dy default primitive restart is not enabled
	primitiveRestartEnable = VK_FALSE;
}


GraphicsPipelineCreateInfo::MultiSampleStateInfo::MultiSampleStateInfo()
{
	sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pNext = NULL;
	flags = 0;
	//we disable multi sample by default
	rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	//Disable by default.For more details https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#primsrast-sampleshading
	sampleShadingEnable = VK_FALSE;
	minSampleShading = 1.0f;
	pSampleMask = NULL;
	alphaToCoverageEnable = VK_FALSE;
	alphaToOneEnable = VK_FALSE;
}

GraphicsPipelineCreateInfo::DepthStencilStateInfo::DepthStencilStateInfo()
{
	sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pNext = NULL;
	flags = 0;
	//by default we set depth test to true
	depthTestEnable = VK_TRUE;
	depthWriteEnable = VK_TRUE;
	depthCompareOp = VK_COMPARE_OP_LESS;
	//by default depth bound test is disabled
	depthBoundsTestEnable = VK_FALSE;
	//by default we don't use stencil test
	stencilTestEnable = VK_FALSE;
	//stencil op state must be setted by user manually if user enables stencil test
	front = VkStencilOpState{};
	back = VkStencilOpState{};
	//max depth bound and min depth bound must be setted by user manually if user enableds depth bound test
	maxDepthBounds = 0.f;
	minDepthBounds = 0.f;
	enable_depth_stencil = false;
}

GraphicsPipelineCreateInfo::RasterizationStateCreateInfo::RasterizationStateCreateInfo()
{
	sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pNext = NULL;
	flags = 0;
	// by default we set  depth clamp enable to false
	// for details see https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#fragops-depth
	depthClampEnable = VK_FALSE;
	// by default we set rasterizer discard enable to false
	// enable this will cause any primitives generated before rasterization will be discarded and nothing will output to frame buffer
	// this is useful when you need some side products from geometry stage and don't want any fragment shader to be executed
	rasterizerDiscardEnable = VK_FALSE;
	// we set fill mode by default
	polygonMode = VK_POLYGON_MODE_FILL;
	// we cull back face triangles by default
	cullMode = VK_CULL_MODE_BACK_BIT;
	// by default clockwise is front face
	frontFace = VK_FRONT_FACE_CLOCKWISE;
	// by default we disable depth bias
	depthBiasEnable = VK_FALSE;
	depthBiasConstantFactor = 0.f;
	depthBiasSlopeFactor = 0.f;
	lineWidth = 0.f;
}


void GraphicsPipelineCreateInfo::DescriptorLayoutHint::AddDescriptorSetLayout(const ptr<gvk::DescriptorSetLayout>& layout)
{
	precluded_descriptor_layouts.push_back(layout);
}


