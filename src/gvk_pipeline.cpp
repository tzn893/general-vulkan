#include "gvk_pipeline.h"
#include "gvk_context.h"

namespace gvk {

	struct DescriptorLayoutInfoHelper
	{
		DescriptorLayoutInfoHelper(const GvkDescriptorLayoutHint& hint,Context& context) 
		:context(context),hint(hint) {
			layout_included.resize(hint.precluded_descriptor_layouts.size());
		}

		//descriptor set layouts
		std::vector<VkDescriptorSetLayout> descriptor_layouts;
		//a bit map check whether the precluded descriptor layouts is included to descriptor layouts for pipeline creation
		std::vector<bool> layout_included;
		std::vector<ptr<DescriptorSetLayout>> internal_layout;

		//Push constant information
		std::vector<VkPushConstantRange> push_constant_ranges;
		std::unordered_map<std::string, VkPushConstantRange> push_constant_table;

		Context& context;
		const GvkDescriptorLayoutHint& hint;

		bool CollectDescriptorLayoutInfo(ptr<Shader> shader) 
		{
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
					auto layout = hint.precluded_descriptor_layouts[i];
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
					auto opt_layout = context.CreateDescriptorSetLayout({ shader }, set->set, nullptr);
					//TODO: what happens if this operation fails? 
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
		}
	};

	
	DescriptorSetLayout::DescriptorSetLayout(VkDescriptorSetLayout layout, const std::vector<ptr<gvk::Shader>>& shaders,
		const std::vector<SpvReflectDescriptorBinding*>& descriptor_set_bindings, uint32 sets,VkDevice device):
	m_Shader(shaders),m_DescriptorSetBindings(descriptor_set_bindings),m_Set(sets),m_Layout(layout) ,m_Device(device),m_ShaderStages(0)
	{
		for (auto shader : shaders) 
		{
			m_ShaderStages |= shader->GetStage();
		}
	}

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

	VkShaderStageFlags DescriptorSetLayout::GetShaderStageBits()
	{
		return m_ShaderStages;
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
					//check if the shader's binding is compatible with existing bindings
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
			vk_bindings[i].descriptorCount = bindings[i]->count;
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


	opt<ptr<Pipeline>> Context::CreateGraphicsPipeline(const GvkGraphicsPipelineCreateInfo& info) {
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
	
		//view port state
		VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
		viewport_state.scissorCount = 1;
		viewport_state.viewportCount = 1;
		vk_create_info.pViewportState = &viewport_state;

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

		std::sort(vertex_input.begin(), vertex_input.end(), 
			[](SpvReflectInterfaceVariable* lhs, SpvReflectInterfaceVariable* rhs) {
				return lhs->location < rhs->location;
			}
		);

		//get rid of gl preserved words
		for (auto iter = vertex_input.begin(); iter < vertex_input.end();)
		{
			std::string name = (*iter)->name;
			if (name.substr(0,3) == "gl_")
			{
				iter = vertex_input.erase(iter);
			}
			else
			{
				iter++;
			}
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
		if (!attributes.empty())
		{
			vertex_input_state.pVertexBindingDescriptions = &binding;
			vertex_input_state.vertexBindingDescriptionCount = 1;
		}
		else
		{
			vertex_input_state.pVertexAttributeDescriptions = NULL;
			vertex_input_state.vertexBindingDescriptionCount = 0;
		}

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

			shader_stage_info.pName = shader->GetEntryPointName();
			//we don't define any specialization
			//for details see https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VkSpecializationInfo
			shader_stage_info.pSpecializationInfo = NULL;
			shader_stage_info.stage = shader->GetStage();
			
			shader_stage_infos.push_back(shader_stage_info);
			return true;
		};

		if (!create_shader_stage(info.vertex_shader)) 
		{
			return std::nullopt;
		}
		if (info.geometry_shader != nullptr) 
		{
			if (!create_shader_stage(info.geometry_shader))
			{
				return std::nullopt;
			}
		}
		if (!create_shader_stage(info.fragment_shader)) 
		{
			return std::nullopt;
		}

		vk_create_info.pStages = shader_stage_infos.data();
		vk_create_info.stageCount = shader_stage_infos.size();

		DescriptorLayoutInfoHelper descriptor_helper(info.descriptor_layuot_hint, *this);
		if (!descriptor_helper.CollectDescriptorLayoutInfo(info.vertex_shader))
		{
			return std::nullopt;
		}
		if (!descriptor_helper.CollectDescriptorLayoutInfo(info.fragment_shader)) {
			return std::nullopt;
		}

		//Render passes
		ptr<RenderPass> target_pass;
		uint32 subpass_index;
		if (info.target_pass != nullptr)
		{
			target_pass   = info.target_pass;
			subpass_index = info.subpass_index;

			vk_create_info.subpass = subpass_index;
			vk_create_info.renderPass = target_pass->GetRenderPass();
		}
		else
		{
			//render passes should be created externally
			return std::nullopt;
		}

		// pipeline layouts
		//We create internal descriptor sets for every shader 
		VkPipelineLayoutCreateInfo pipeline_layout_create_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipeline_layout_create_info.flags = 0;
		pipeline_layout_create_info.pSetLayouts				= descriptor_helper.descriptor_layouts.data();
		pipeline_layout_create_info.setLayoutCount			= descriptor_helper.descriptor_layouts.size();
		pipeline_layout_create_info.pPushConstantRanges		= descriptor_helper.push_constant_ranges.data();
		pipeline_layout_create_info.pushConstantRangeCount	= descriptor_helper.push_constant_ranges.size();

		VkPipelineLayout pipeline_layout;
		if (vkCreatePipelineLayout(m_Device, &pipeline_layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
		{
			return std::nullopt;
		}

		vk_create_info.layout = pipeline_layout;

		VkPipeline pipeline;
		if (vkCreateGraphicsPipelines(m_Device,NULL,1,&vk_create_info,nullptr,&pipeline) != VK_SUCCESS) 
		{
			vkDestroyPipelineLayout(m_Device, pipeline_layout, nullptr);
			return std::nullopt;
		}
		
		return ptr<Pipeline>(new Pipeline(pipeline,pipeline_layout,
			descriptor_helper.internal_layout,descriptor_helper.push_constant_table,
			target_pass,subpass_index,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Device));
	}

	opt<ptr<gvk::Pipeline>> Context::CreateComputePipeline(const GvkComputePipelineCreateInfo& info)
	{
		VkComputePipelineCreateInfo create_info{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
		if (auto v = info.shader->GetShaderModule(); v.has_value()) 
		{
			create_info.stage.module = v.value();
		}
		else 
		{
			return std::nullopt;
		}

		create_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		create_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		create_info.stage.pName = info.shader->GetEntryPointName();
		
		DescriptorLayoutInfoHelper helper(info.descriptor_layuot_hint, *this);
		if (!helper.CollectDescriptorLayoutInfo(info.shader)) 
		{
			return std::nullopt;
		}

		VkPipelineLayoutCreateInfo pipeline_layout_create{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipeline_layout_create.setLayoutCount = helper.descriptor_layouts.size();
		pipeline_layout_create.pSetLayouts = helper.descriptor_layouts.data();
		pipeline_layout_create.pushConstantRangeCount = helper.push_constant_ranges.size();
		pipeline_layout_create.pPushConstantRanges = helper.push_constant_ranges.data();
		pipeline_layout_create.flags = 0;

		VkPipelineLayout layout;
		if (vkCreatePipelineLayout(m_Device,&pipeline_layout_create,nullptr,&layout) != VK_SUCCESS) 
		{
			return std::nullopt;
		}
		create_info.layout = layout;

		VkPipeline compute_pipeline;
		if (vkCreateComputePipelines(m_Device,NULL,1,&create_info,nullptr,&compute_pipeline) != VK_SUCCESS) 
		{
			vkDestroyPipelineLayout(m_Device, layout, nullptr);
			return std::nullopt;
		}

		return ptr<Pipeline>(new Pipeline(compute_pipeline, layout,
			helper.internal_layout, helper.push_constant_table,
			nullptr,0,VK_PIPELINE_BIND_POINT_COMPUTE, m_Device));
	}

	opt<ptr<gvk::RenderPass>> Context::CreateRenderPass(const GvkRenderPassCreateInfo& info)
	{
		gvk_assert(m_Device != NULL);
		VkRenderPass pass;
		if (vkCreateRenderPass(m_Device, &info, nullptr, &pass) != VK_SUCCESS) 
		{
			return std::nullopt;
		}
		return ptr<RenderPass>(new RenderPass(pass,m_Device,info.subpassCount,info.attachmentCount));
	}

	uint32 RenderPass::GetAttachmentCount()
	{
		return m_AttachmentCount;
	}

	uint32 RenderPass::GetSubpassCount()
	{
		return m_SubpassCount;
	}

	VkRenderPass RenderPass::GetRenderPass()
	{
		return m_Pass;
	}

	gvk::RenderPassInlineContent RenderPass::Begin(VkFramebuffer framebuffer, VkClearValue* clear_values, VkRect2D render_area,
		VkViewport viewport, VkRect2D sissor, VkCommandBuffer command_buffer)
	{
		VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		begin.clearValueCount = m_AttachmentCount;
		begin.pClearValues = clear_values;
		begin.framebuffer = framebuffer;
		begin.renderArea = render_area;
		begin.renderPass = m_Pass;
		
		vkCmdBeginRenderPass(command_buffer, &begin, VK_SUBPASS_CONTENTS_INLINE);

		//we don't support multiple viewport now
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &sissor);

		return RenderPassInlineContent(framebuffer, command_buffer);
	}

	RenderPass::~RenderPass()
	{
		vkDestroyRenderPass(m_Device, m_Pass, nullptr);
	}

	RenderPass::RenderPass(VkRenderPass render_pass, VkDevice device, uint32 subpass_count,
		uint32 attachment_count)
	:m_Device(device),m_Pass(render_pass),m_SubpassCount(subpass_count),m_AttachmentCount(attachment_count) {}

	opt<ptr<RenderPass>> Pipeline::GetRenderPass()
	{
		if (m_RenderPass != nullptr) 
		{
			return m_RenderPass;
		}
		return std::nullopt;
	}

	opt<ptr<gvk::DescriptorSetLayout>> Pipeline::GetInternalLayout(uint32 set, VkShaderStageFlagBits stage)
	{
		for (auto& layout : m_InternalDescriptorSetLayouts) 
		{
			if (layout->GetSetID() == set && (layout->GetShaderStageBits() & stage) == stage) 
			{
				return layout;
			}
		}
		return std::nullopt;
	}

	VkPipeline Pipeline::GetPipeline()
	{
		return m_Pipeline;
	}

	VkPipelineLayout Pipeline::GetPipelineLayout()
	{
		return m_PipelineLayout;
	}

	VkPipelineBindPoint Pipeline::GetPipelineBindPoint()
	{
		return m_BindPoint;
	}

	opt<GvkPushConstant> Pipeline::GetPushConstantRange(const char* name)
	{
		if (auto res = m_PushConstants.find(name); res != m_PushConstants.end()) 
		{
			GvkPushConstant constant(res->second, m_PipelineLayout);
			return constant;
		}
		return std::nullopt;
	}

	Pipeline::~Pipeline()
	{
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
	}

	Pipeline::Pipeline(VkPipeline pipeline, VkPipelineLayout layout, const std::vector<ptr<DescriptorSetLayout>>& descriptor_set_layouts, const std::unordered_map<std::string,VkPushConstantRange>& push_constants, ptr<RenderPass> render_pass,uint32 subpass_index,VkPipelineBindPoint bind_point,VkDevice device) 
		:m_Pipeline(pipeline),m_PipelineLayout(layout),m_InternalDescriptorSetLayouts(descriptor_set_layouts),m_PushConstants(push_constants),
	m_RenderPass(render_pass),m_SubpassIndex(subpass_index),m_Device(device),m_BindPoint(bind_point) 
	{}

	VkDescriptorSet DescriptorSet::GetDescriptorSet()
	{
		return m_Set;
	}

	uint32 DescriptorSet::GetSetIndex()
	{
		return m_Layout->GetSetID();
	}

	opt<SpvReflectDescriptorBinding*> DescriptorSet::FindBinding(const char* name)
	{
		auto bindings = m_Layout->GetDescriptorSetBindings();
		for (uint32 i = 0; i < bindings.size(); i++)
		{
			if (strcmp(bindings[i]->name, name) == 0)
			{
				return bindings[i];
			}
		}
		return std::nullopt;
	}

	DescriptorSet::~DescriptorSet()
	{
		//descriptor set will be released automatically when descriptor pool is released
	}

	DescriptorSet::DescriptorSet(VkDevice device, VkDescriptorSet set, ptr<DescriptorSetLayout> layout)
		:m_Device(device),m_Set(set),m_Layout(layout)
	{}

	opt<ptr<gvk::DescriptorSet>> DescriptorAllocator::Allocate(ptr<DescriptorSetLayout> layout)
	{
		//collect descriptor informations
		auto bindings = layout->GetDescriptorSetBindings();
		for (uint32 i = 0;i < bindings.size();i++) 
		{
			m_PoolSize[bindings[i]->descriptor_type].descriptorCount += bindings[i]->count;
		}
		m_MaxSetCount++;

		//allocate descriptor set
		VkDescriptorSetLayout layouts[] = {layout->GetLayout()};
		VkDescriptorSetAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		alloc_info.descriptorPool = m_CurrentDescriptorPool;
		alloc_info.descriptorSetCount = gvk_count_of(layouts);
		alloc_info.pSetLayouts = layouts;

		VkDescriptorSet set;
		if (vkAllocateDescriptorSets(m_Device,&alloc_info,&set) != VK_SUCCESS) 
		// current pool may run out of space
		// create a new one
		{
			m_UsedDescriptorPool.push_back(m_CurrentDescriptorPool);
			if (auto v = CreatePool(); !v.has_value()) 
			{
				m_CurrentDescriptorPool = v.value();
			}
			else
			{
				return std::nullopt;
			}
			auto vkrs = vkAllocateDescriptorSets(m_Device, &alloc_info, &set);
			gvk_assert(vkrs == VK_SUCCESS);
		}

		return ptr<DescriptorSet>(new DescriptorSet(m_Device, set, layout));
	}

	ptr<gvk::DescriptorAllocator> Context::CreateDescriptorAllocator()
	{
		return ptr<gvk::DescriptorAllocator>(new DescriptorAllocator(m_Device));
	}

	DescriptorAllocator::~DescriptorAllocator()
	{
		for (auto pool : m_UsedDescriptorPool) 
		{
			vkDestroyDescriptorPool(m_Device, pool, nullptr);
		}
		vkDestroyDescriptorPool(m_Device, m_CurrentDescriptorPool, nullptr);
	}

	constexpr uint32 pool_size = 1024;
	static std::vector<VkDescriptorPoolSize> g_pool_sizes =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER				,  pool_size / 2 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER	, 4 * pool_size },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE			, 4 * pool_size },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE			, 1 * pool_size },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER	, 1 * pool_size},
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER	, 1 * pool_size},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER			, 2 * pool_size},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER			, 2 * pool_size},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC	, 1 * pool_size},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC	, 1 * pool_size},
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT		, pool_size / 2}
	};

	DescriptorAllocator::DescriptorAllocator(VkDevice device)
		:m_Device(device) 
	{
		m_MaxSetCount = pool_size / 4;
		m_PoolSize = g_pool_sizes;
		auto pool = CreatePool();
		gvk_assert(pool.has_value());
		m_CurrentDescriptorPool = pool.value();
	}

	opt<VkDescriptorPool> DescriptorAllocator::CreatePool()
	{
		std::vector<VkDescriptorPoolSize> target_pools;
		for (auto& size : m_PoolSize) 
		{
			if (size.descriptorCount != 0) 
			{
				target_pools.push_back(size);
			}
			size.descriptorCount = 0;
		}

		VkDescriptorPoolCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		info.poolSizeCount = target_pools.size();
		info.pPoolSizes = target_pools.data();
		info.maxSets = m_MaxSetCount;

		m_MaxSetCount = 0;

		VkDescriptorPool pool;
		if (vkCreateDescriptorPool(m_Device, &info, nullptr, &pool) != VK_SUCCESS)
		{
			return std::nullopt;
		}

		return pool;
	}

	void RenderPassInlineContent::Record(std::function<void()> commands)
	{
		commands();

		vkCmdEndRenderPass(m_CommandBuffer);
	}

	RenderPassInlineContent& RenderPassInlineContent::NextSubPass(std::function<void()> commands)
	{
		commands();

		vkCmdNextSubpass(m_CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		return *this;
	}

	void RenderPassInlineContent::EndPass(std::function<void()> commands)
	{
		commands();

		vkCmdEndRenderPass(m_CommandBuffer);
	}

	RenderPassInlineContent::RenderPassInlineContent(VkFramebuffer framebuffer, VkCommandBuffer command_buffer)
		:m_Framebuffer(framebuffer),m_CommandBuffer(command_buffer) {}
}

using namespace gvk;

GvkGraphicsPipelineCreateInfo::FrameBufferBlendState::FrameBufferBlendState()
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

void GvkGraphicsPipelineCreateInfo::FrameBufferBlendState::Resize(uint32 frame_buffer_count)
{
	//create a default blend state for every resized states
	VkPipelineColorBlendAttachmentState default_blend_state = BlendState();
	frame_buffer_states.resize(frame_buffer_count, default_blend_state);

	create_info.attachmentCount = frame_buffer_count;
	create_info.pAttachments = frame_buffer_states.data();
}

void GvkGraphicsPipelineCreateInfo::FrameBufferBlendState::Set(uint32 index, const BlendState& state)
{
	gvk_assert(index < frame_buffer_states.size());
	frame_buffer_states[index] = state;
}


GvkGraphicsPipelineCreateInfo::BlendState::BlendState()
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

GvkGraphicsPipelineCreateInfo::InputAssembly::InputAssembly()
{
	sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pNext = NULL;
	flags = 0;
	// most primitive we draw are triangle list
	topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// dy default primitive restart is not enabled
	primitiveRestartEnable = VK_FALSE;
}


GvkGraphicsPipelineCreateInfo::MultiSampleStateInfo::MultiSampleStateInfo()
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

GvkGraphicsPipelineCreateInfo::DepthStencilStateInfo::DepthStencilStateInfo()
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
	//stencil op state must be set by user manually if user enables stencil test
	front = VkStencilOpState{};
	back = VkStencilOpState{};
	//max depth bound and min depth bound must be set by user manually if user enables depth bound test
	maxDepthBounds = 1.f;
	minDepthBounds = 0.f;
	enable_depth_stencil = false;
}

GvkGraphicsPipelineCreateInfo::RasterizationStateCreateInfo::RasterizationStateCreateInfo()
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
	depthBiasClamp = 0.f;
	//line width member must be 1.0f
	//see https://vulkan.lunarg.com/doc/view/1.3.216.0/windows/1.3-extensions/vkspec.html#VUID-VkGraphicsPipelineCreateInfo-pDynamicStates-00749
	lineWidth = 1.f;
}

GvkGraphicsPipelineCreateInfo::GvkGraphicsPipelineCreateInfo(ptr<gvk::Shader> vert, ptr<gvk::Shader> frag, ptr<gvk::RenderPass> render_pass, uint32 subpass_index,
	const GvkGraphicsPipelineCreateInfo::BlendState* blend_states)
 :vertex_shader(vert),fragment_shader(frag),target_pass(render_pass),subpass_index(subpass_index) 
{	
	uint32 fragment_output_count = frag->GetOutputVariableCount();
	frame_buffer_blend_state.Resize(fragment_output_count);

	if (blend_states == NULL && fragment_output_count != 0)
	{
		static std::vector<GvkGraphicsPipelineCreateInfo::BlendState> s_blend_states;
		s_blend_states.resize(fragment_output_count, GvkGraphicsPipelineCreateInfo::BlendState());
		blend_states = s_blend_states.data();
	}

	for (uint32 i = 0;i < fragment_output_count;i++)
	{
		frame_buffer_blend_state.Set(i, blend_states[i]);
	}
}

void GvkDescriptorLayoutHint::AddDescriptorSetLayout(const ptr<gvk::DescriptorSetLayout>& layout)
{
	precluded_descriptor_layouts.push_back(layout);
}


GvkRenderPassCreateInfo::GvkRenderPassCreateInfo()
{
	sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	pNext = NULL;
	//currently we ignore the render pass create flag
	flags = 0;
	
	attachmentCount = 0;
	pAttachments = NULL;
	subpassCount = 0;
	pSubpasses = NULL;
	dependencyCount = 0;
	pDependencies = NULL;

	//we assume depth stencil attachment is less than 1024
	m_SubpassDepthReference.reserve(1024);
}

uint32 GvkRenderPassCreateInfo::AddAttachment(VkAttachmentDescriptionFlags flag, VkFormat format, VkSampleCountFlagBits sample_counts, VkAttachmentLoadOp load, VkAttachmentStoreOp store, VkAttachmentLoadOp stencil_load, VkAttachmentStoreOp stencil_store, VkImageLayout init_layout, VkImageLayout end_layout)
{
	m_Attachment.push_back( 
		VkAttachmentDescription{flag,format,sample_counts,load,store,stencil_load,stencil_store,init_layout,end_layout}
	);
	attachmentCount = m_Attachment.size();
	pAttachments	= m_Attachment.data();

	return m_Attachment.size() - 1;
}

uint32_t GvkRenderPassCreateInfo::AddSubpass(VkSubpassDescriptionFlags flags, VkPipelineBindPoint bind_point)
{
	VkSubpassDescription desc{};
	desc.flags = flags;
	desc.pipelineBindPoint = bind_point;
	
	m_Subpasses.push_back(desc);
	m_SubpassColorReference.push_back({});
	m_SubpassDepthReference.push_back({});
	m_SubpassInputReference.push_back({});

	subpassCount = m_Subpasses.size();
	pSubpasses = m_Subpasses.data();

	return m_Subpasses.size() - 1;
}

void GvkRenderPassCreateInfo::AddSubpassColorAttachment(uint32 subpass_index, uint32 attachment_index)
{
	gvk_assert(subpass_index < m_Subpasses.size());
	gvk_assert(attachment_index < m_Attachment.size());

	auto& subpass = m_Subpasses[subpass_index];
	auto& subpass_attachments = m_SubpassColorReference[subpass_index];

	VkAttachmentReference ref{};
	ref.attachment = attachment_index;
	ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	subpass_attachments.push_back(ref);

	subpass.colorAttachmentCount = subpass_attachments.size();
	subpass.pColorAttachments = subpass_attachments.data();
}

void GvkRenderPassCreateInfo::AddSubpassDepthStencilAttachment(uint32 subpass_index, uint32 attachment_index)
{
	gvk_assert(subpass_index < m_Subpasses.size());
	gvk_assert(attachment_index < m_Attachment.size());

	auto& subpass = m_Subpasses[subpass_index];
	auto& subpass_attachment = m_SubpassDepthReference[subpass_index];

	VkAttachmentReference ref{};
	ref.attachment = attachment_index;
	ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	subpass_attachment = ref;
	subpass.pDepthStencilAttachment = &subpass_attachment;
}

void GvkRenderPassCreateInfo::AddSubpassInputAttachment(uint32 subpass_index, uint32 attachment_index, VkImageLayout layout)
{
	gvk_assert(subpass_index < m_Subpasses.size());
	gvk_assert(attachment_index < m_Attachment.size());

	auto& subpass = m_Subpasses[subpass_index];
	auto& subpass_attachments = m_SubpassInputReference[subpass_index];

	VkAttachmentReference ref{};
	ref.attachment = attachment_index;
	ref.layout = layout;

	subpass_attachments.push_back(ref);

	subpass.inputAttachmentCount = subpass_attachments.size();
	subpass.pInputAttachments = subpass_attachments.data();
}

void GvkRenderPassCreateInfo::AddSubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkDependencyFlags dependencyFlags)
{
	m_Dependencies.push_back(VkSubpassDependency{
			srcSubpass,
			dstSubpass,
			srcStageMask,
			dstStageMask,
			srcAccessMask,
			dstAccessMask,
			dependencyFlags
		});

	pDependencies = m_Dependencies.data();
	dependencyCount = m_Dependencies.size();
}

GvkDescriptorSetWrite& GvkDescriptorSetWrite::BufferWrite(ptr<gvk::DescriptorSet> set,VkDescriptorType descriptor_type,uint32 binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, uint32 array_index /*= 0*/)
{
	buffers.push_back(GvkDescriptorSetBufferWrite{ set->GetDescriptorSet(),descriptor_type,binding,array_index,buffer,offset,size});
	return *this;
}

GvkDescriptorSetWrite& GvkDescriptorSetWrite::BufferWrite(ptr<gvk::DescriptorSet> set, const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, uint32 array_index /*= 0*/)
{
	auto binding = set->FindBinding(name);
	if (binding.has_value()) 
	{
		BufferWrite(set,(VkDescriptorType)binding.value()->descriptor_type, binding.value()->binding, buffer, offset, size, array_index);
	}
	return *this;
}

void GvkDescriptorSetWrite::Emit(VkDevice device)
{
	std::vector<VkWriteDescriptorSet> writes;

	std::vector<VkDescriptorBufferInfo> buffer_infos(buffers.size());
	std::vector<VkDescriptorImageInfo>	image_infos(images.size());
	for (uint32 i = 0; i < buffers.size(); i++)
	{
		auto& buffer = buffers[i];

		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorType = buffer.type;
		//TODO : currently we only support write one buffer at a time
		write.descriptorCount = 1;
		write.dstArrayElement = buffer.array_index;
		write.dstBinding = buffer.binding;
		write.dstSet = buffer.desc_set;

		VkDescriptorBufferInfo buffer_info;
		buffer_info.buffer = buffer.buffer;
		buffer_info.offset = buffer.offset;
		buffer_info.range = buffer.size;

		buffer_infos[i] = buffer_info;

		write.pBufferInfo = buffer_infos.data() + i;

		writes.push_back(write);
	}

	for (uint32 i = 0; i < images.size(); i++)
	{
		auto& image = images[i];

		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorType = (VkDescriptorType)image.type;
		//TODO : currently we only support write one buffer at a time
		write.descriptorCount = 1;
		write.dstArrayElement = image.array_index;
		write.dstBinding = image.binding;
		write.dstSet = image.desc_set;

		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = image.layout;
		image_info.imageView = image.image;
		image_info.sampler = image.sampler;

		image_infos[i] = image_info;

		write.pImageInfo = image_infos.data() + i;

		writes.push_back(write);
	}

	vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, NULL);
}

GvkDescriptorSetWrite& GvkDescriptorSetWrite::ImageWrite(ptr<gvk::DescriptorSet> set, VkDescriptorType descriptor_type, uint32 binding, VkSampler sampler, VkImageView image_view, VkImageLayout layout, uint32 array_index /*= 0*/)
{
	images.push_back(GvkDescriptorSetImageWrite{ set->GetDescriptorSet(),descriptor_type,binding,array_index,sampler,image_view,layout});
	return *this;
}

GvkDescriptorSetWrite& GvkDescriptorSetWrite::ImageWrite(ptr<gvk::DescriptorSet> set, const char* name, VkSampler sampler, VkImageView image_view, VkImageLayout layout, uint32 array_index /*= 0*/)
{
	auto binding = set->FindBinding(name);
	if (binding.has_value())
	{
		ImageWrite(set,(VkDescriptorType)binding.value()->descriptor_type, binding.value()->binding, sampler, image_view, layout, array_index);
	}
	return *this;
}

void GvkPushConstant::Update(VkCommandBuffer cmd_buffer,const void* data)
{
	vkCmdPushConstants(cmd_buffer, layout, range.stageFlags, range.offset, range.size, data);
}

GvkPushConstant::GvkPushConstant(VkPushConstantRange range, VkPipelineLayout layout)
:range(range),layout(layout) {}


GvkDescriptorSetBindingUpdate::GvkDescriptorSetBindingUpdate(VkCommandBuffer cmd_buffer, gvk::ptr<gvk::Pipeline> pipeline)
{
	descriptor_set_count = 0;

	this->cmd_buffer = cmd_buffer;
	this->bind_point = pipeline->GetPipelineBindPoint();
	this->layout = pipeline->GetPipelineLayout();
}

GvkDescriptorSetBindingUpdate& GvkDescriptorSetBindingUpdate::BindDescriptorSet(gvk::ptr<gvk::DescriptorSet> set)
{
	target_sets[descriptor_set_count++] = set;
	return *this;
}

void GvkDescriptorSetBindingUpdate::Update()
{
	//target descriptor sets should not be empty!
	gvk_assert(descriptor_set_count != 0);

	if (descriptor_set_count == 1)
	{
		VkDescriptorSet set = target_sets[0]->GetDescriptorSet();
		vkCmdBindDescriptorSets(cmd_buffer, bind_point, layout, target_sets[0]->GetSetIndex(),
			descriptor_set_count, &set, 0, NULL);
		return;
	}

	std::sort(target_sets.begin(), target_sets.begin() + descriptor_set_count,
		[&](const gvk::ptr<gvk::DescriptorSet>& lhs, const gvk::ptr<gvk::DescriptorSet>& rhs)
		{
			return lhs->GetDescriptorSet() < rhs->GetDescriptorSet();
		}
	);

	std::array<VkDescriptorSet, 16> sets;
	
	sets[0] = target_sets[0]->GetDescriptorSet();
	uint32	batch_set_cnt = 1;
	uint32	batch_last_set_idx = target_sets[0]->GetSetIndex();
	uint32	batch_first_set_idx = target_sets[0]->GetSetIndex();
	
	for (uint32 i = 1; i < descriptor_set_count; i++)
	{
		ptr<gvk::DescriptorSet> curr_set = target_sets[i];
		if (curr_set->GetSetIndex() != batch_last_set_idx + 1)
		{
			vkCmdBindDescriptorSets(cmd_buffer, bind_point, layout, batch_first_set_idx,
				batch_set_cnt, sets.data(), 0, NULL);

			batch_set_cnt = 1;
			batch_first_set_idx = curr_set->GetSetIndex();
			batch_last_set_idx = curr_set->GetSetIndex();
			sets[0] = curr_set->GetDescriptorSet();
		}
		else
		{
			batch_last_set_idx = curr_set->GetSetIndex();
			sets[batch_set_cnt++] = curr_set->GetDescriptorSet();
		}
	}
	vkCmdBindDescriptorSets(cmd_buffer, bind_point, layout, batch_first_set_idx,
		batch_set_cnt, sets.data(), 0, NULL);
}
