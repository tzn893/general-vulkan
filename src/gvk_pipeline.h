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
		uint32 GetSetID() ;
		bool   CreatedFromShader(const ptr<gvk::Shader>& shader,uint32 set);
		View<SpvReflectDescriptorBinding*> GetDescriptorSetBindings();

		VkShaderStageFlags		GetShaderStageBits();

		~DescriptorSetLayout();
	private:
		DescriptorSetLayout(VkDescriptorSetLayout layout,const std::vector<ptr<gvk::Shader>>& shaders,
			const std::vector<SpvReflectDescriptorBinding*>& descriptor_set_bindings,uint32 sets,VkDevice device);
		
		VkShaderStageFlags							m_ShaderStages;
		VkDescriptorSetLayout						m_Layout;
		std::vector<ptr<gvk::Shader>>				m_Shader;
		std::vector<SpvReflectDescriptorBinding*>	m_DescriptorSetBindings;
		uint32										m_Set;
		VkDevice									m_Device;
	};

	class RenderPass {
		friend class Context;
	public:
		uint32 GetSubpassCount();
		VkRenderPass GetRenderPass();

		~RenderPass();
	private:
		RenderPass(VkRenderPass render_pass,VkDevice device,uint32 subpass_count);

		VkRenderPass m_Pass;
		VkDevice	 m_Device;

		uint32 m_SubpassCount;
	};
}

struct GvkDescriptorLayoutHint
{
	void AddDescriptorSetLayout(const ptr<gvk::DescriptorSetLayout>& layout);
	std::vector<ptr<gvk::DescriptorSetLayout>> precluded_descriptor_layouts;

};

struct GvkGraphicsPipelineCreateInfo {
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


	GvkDescriptorLayoutHint descriptor_layuot_hint;


	ptr<gvk::RenderPass>	target_pass;
	uint32					subpass_index = 0;
};


struct GvkComputePipelineCreateInfo 
{
	ptr<gvk::Shader>		shader;

	GvkDescriptorLayoutHint	descriptor_layuot_hint;

	ptr<gvk::RenderPass>	render_pass;
	uint32					subpass_index;
};

struct GvkRenderPassCreateInfo : public VkRenderPassCreateInfo
{
	GvkRenderPassCreateInfo();

	uint32	AddAttachment(VkAttachmentDescriptionFlags flag,VkFormat format,
		VkSampleCountFlagBits sample_counts,VkAttachmentLoadOp load,
		VkAttachmentStoreOp store,VkAttachmentLoadOp stencil_load,VkAttachmentStoreOp stencil_store,
		VkImageLayout init_layout,VkImageLayout end_layout);

	void	AddSubpass(VkSubpassDescriptionFlags flags, VkPipelineBindPoint bind_point);
	void	AddSubpassColorAttachment(uint32 subpass_index,uint32 attachment_index);
	void	AddSubpassDepthStencilAttachment(uint32 subpass_index,uint32 attachment_index);
	void	AddSubpassInputAttachment(uint32 subpass_index,uint32 attachment_index,VkImageLayout layout);

	void	AddSubpassDependency(uint32_t                srcSubpass,
								 uint32_t                dstSubpass,
							     VkPipelineStageFlags    srcStageMask,
								 VkPipelineStageFlags    dstStageMask,
								 VkAccessFlags           srcAccessMask,
								 VkAccessFlags           dstAccessMask,
								 VkDependencyFlags       dependencyFlags);

	std::vector<VkSubpassDescription>				m_Subpasses;
	std::vector<VkAttachmentReference>				m_SubpassDepthReference;
	std::vector<std::vector<VkAttachmentReference>>	m_SubpassColorReference;
	std::vector<std::vector<VkAttachmentReference>> m_SubpassInputReference;
	std::vector<VkAttachmentDescription>			m_Attachment;
	std::vector<VkSubpassDependency>				m_Dependencies;
};

struct GvkDescriptorSetBufferWrite 
{
	const char*			name;
	uint32				array_index;
	VkBuffer			buffer;
	VkDeviceSize		offset;
	VkDeviceSize		size;
};

struct GvkDescriptorSetImageWrite 
{
	const char*		name;
	uint32			array_index;
	VkSampler		sampler;
	VkImageView		image;
	VkImageLayout	layout;
};

struct GvkDescriptorSetWrite 
{
	std::vector<GvkDescriptorSetBufferWrite> buffers;
	std::vector<GvkDescriptorSetImageWrite>  images;

	GvkDescriptorSetWrite& Buffer(const char* name,VkBuffer buffer,VkDeviceSize offset, VkDeviceSize size,uint32 array_index = 0);
	GvkDescriptorSetWrite& Image(const char* name,VkSampler sampler,VkImageView image_view,VkImageLayout layout,uint32 array_index = 0);
};

namespace gvk{
	//It's recommended that one thread one descriptor allocator
	class DescriptorAllocator 
	{
		friend class DescriptorSet;
		friend class Context;
	public:
		
		opt<ptr<DescriptorSet>> Allocate(ptr<DescriptorSetLayout> layout);

		~DescriptorAllocator();

	private:
		DescriptorAllocator(VkDevice device);

		opt<VkDescriptorPool>	CreatePool();

		//TODO : release the descriptor pools that don't used any more
		std::vector<VkDescriptorPool> m_UsedDescriptorPool;
		VkDescriptorPool			  m_CurrentDescriptorPool;

		//	this allocator is self-adaptive, will change the pool size
		//	according to the last pool
		std::vector<VkDescriptorPoolSize> m_PoolSize;

		uint32							  m_MaxSetCount;
		VkDevice						  m_Device;
	};

	class DescriptorSet 
	{
		friend class DescriptorAllocator;
	public:
		VkDescriptorSet GetDescriptorSet();
		uint32			GetSetIndex();

		void Write(const GvkDescriptorSetWrite& write);
		~DescriptorSet();

	private:
		DescriptorSet(VkDevice device,VkDescriptorSet set,ptr<DescriptorSetLayout> layout);

		VkDevice							m_Device;
		VkDescriptorSet						m_Set;
		ptr<DescriptorSetLayout>			m_Layout;
	};


	class Pipeline {
		friend class Context;
	public:
		opt<ptr<RenderPass>>					GetRenderPass();
		opt<ptr<DescriptorSetLayout>>			GetInternalLayout(uint32 set,VkShaderStageFlagBits stage);

		VkPipeline								GetPipeline();
		VkPipelineLayout						GetPipelineLayout();

		~Pipeline();
	private:
		Pipeline(VkPipeline pipeline, VkPipelineLayout layout, const std::vector<ptr<DescriptorSetLayout>>& descriptor_set_layouts, 
			const std::unordered_map<std::string,VkPushConstantRange>& push_constants, ptr<RenderPass> render_pass,uint32 subpass_index,VkDevice device);

		VkDevice												m_Device;
		VkPipeline												m_Pipeline;
		VkPipelineLayout										m_PipelineLayout;
		//descriptor sets not created from layout hints will be created internally
		std::vector<ptr<DescriptorSetLayout>>					m_InternalDescriptorSetLayouts;
		std::unordered_map<std::string, VkPushConstantRange>	m_PushConstants;
		ptr<RenderPass>											m_RenderPass;
		uint32													m_SubpassIndex;
	};
	
}