#pragma once
#include "gvk_common.h"
#include "gvk_shader.h"
#include "shader/shader_common.h"

namespace gvk {
	//descriptor set layout is created from 
	class DescriptorSetLayout {
		friend class Context;
	public:
		VkDescriptorSetLayout GetLayout();
		uint32 GetSetID();
		bool   CreatedFromShader(const ptr<gvk::Shader>& shader,uint32 set);
		View<SpvReflectDescriptorBinding*> GetDescriptorSetBindings();

		~DescriptorSetLayout();
	private:
		DescriptorSetLayout(VkDescriptorSetLayout layout,const std::vector<ptr<gvk::Shader>>& shaders,
			const std::vector<SpvReflectDescriptorBinding*>& descriptor_set_bindings,uint32 sets,VkDevice device);

		VkDescriptorSetLayout m_Layout;
		std::vector<ptr<gvk::Shader>> m_Shader;
		std::vector<SpvReflectDescriptorBinding*> m_DescriptorSetBindings;
		uint32 m_Set;
		VkDevice m_Device;
	};
}

struct GraphicsPipelineCreateInfo {
	ptr<gvk::Shader> vertex_shader;

	//TODO : currently we don't support tessellation or geometry stage 
	//ptr<gvk::Shader> tess_control_shader;
	//ptr<gvk::Shader> tess_evalue_shader;
	//ptr<gvk::Shader> geometry_shader;

	ptr<gvk::Shader> fragment_shader;

	//TODO : currently we don't support mesh shader work flow
	//ptr<gvk::Shader> task_shader;
	//ptr<gvk::Shader> mesh_shader;
	
	struct RasterizationStateCreateInfo : public VkPipelineRasterizationStateCreateInfo {
		//constructor will set usually used parameters automatically
		RasterizationStateCreateInfo();
	} rasterization_state;

	struct DepthStencilStateInfo : public VkPipelineDepthStencilStateCreateInfo {
		//constructor will set usually used parameters automatically
		DepthStencilStateInfo();
		bool enable_depth_stencil;
	} depth_stencil_state;


	struct MultiSampleStateInfo : public VkPipelineMultisampleStateCreateInfo {
		//constructor will set usually used parameters automatically
		//by default multi sample state is not enabled
		MultiSampleStateInfo();
	} multi_sample_state;


	struct BlendState : public VkPipelineColorBlendAttachmentState {
		//constructor will set usually used parameters automatically
		BlendState();
	};

	struct FrameBufferBlendState {
		FrameBufferBlendState();

		/// <summary>
		/// Resize the blend states to frame buffer count of blend states
		/// </summary>
		/// <param name="frame_buffer_count"></param>
		void Resize(uint32 frame_buffer_count);

		/// <summary>
		/// set the blend state at specified location
		/// </summary>
		/// <param name="index">index of blend state</param>
		/// <param name="state">state to set</param>
		void Set(uint32 index,const BlendState& state);

		std::vector<VkPipelineColorBlendAttachmentState> frame_buffer_states;
		VkPipelineColorBlendStateCreateInfo create_info;
	} frame_buffer_blend_state;

	struct InputAssembly : public VkPipelineInputAssemblyStateCreateInfo {
		//constructor will set usually used parameters automatically
		InputAssembly();
	} input_assembly_state;


	struct DescriptorLayoutHint 
	{
		void AddDescriptorSetLayout(const ptr<gvk::DescriptorSetLayout>& layout);		
		std::vector<ptr<gvk::DescriptorSetLayout>> precluded_descriptor_layouts;

	} descriptor_layuot_hint;
};


namespace gvk{

	class DescriptorSet {
	public:

	private:

	};


	class GraphicsPipeline {
		friend class Context;
	public:
		

		VkPipeline								GetPipeline();
		const GraphicsPipelineCreateInfo&		Info() const;
	private:
		VkPipeline												m_Pipeline;
		VkPipelineLayout										m_PipelineLayout;
		GraphicsPipelineCreateInfo								m_CreateInfo;
		//descriptor sets not created from layout hints will be created internally
		std::vector<ptr<DescriptorSetLayout>>					m_InteralDescriptorSetLayouts;
		std::unordered_map<std::string, VkPushConstantRange>	m_PushConstants;
	};

	class ComputePipeline {
	public:

	private:

	};

	class RaytracingPipeline {
	public:
	private:
	};
}