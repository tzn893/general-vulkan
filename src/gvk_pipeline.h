#pragma once
#include "gvk_common.h"
#include "gvk_shader.h"
#include "gvk_shader_common.h"
#include <functional>

namespace gvk {
	class TopAccelerationStructure;
	
	//descriptor set layout is created from 
	class DescriptorSetLayout {
		friend class Context;
	public:
		VkDescriptorSetLayout GetLayout();
		uint32_t GetSetID() ;
		bool   CreatedFromShader(const gvk::ptr<gvk::Shader>& shader,uint32_t set);
		View<SpvReflectDescriptorBinding*> GetDescriptorSetBindings();

		VkShaderStageFlags		GetShaderStageBits();

		uint32_t				GetMaxBindlessDescriptorSetCount();
		bool					IsBindless();

		~DescriptorSetLayout();
	private:
		DescriptorSetLayout(VkDescriptorSetLayout layout,const std::vector<gvk::ptr<gvk::Shader>>& shaders,
			const std::vector<SpvReflectDescriptorBinding*>& descriptor_set_bindings,uint32_t sets,VkDevice device,
			uint32_t maxBindlessBindingCount, bool isBindless);
		
		VkShaderStageFlags							m_ShaderStages;
		VkDescriptorSetLayout						m_Layout;
		std::vector<gvk::ptr<gvk::Shader>>			m_Shader;
		std::vector<SpvReflectDescriptorBinding*>	m_DescriptorSetBindings;
		uint32_t									m_Set;
		VkDevice									m_Device;

		uint32_t									m_MaxBindlessBindingCount;
		bool										m_IsBindless;
	};

	class RenderPassInlineContent 
	{
		friend class RenderPass;
	public:
		void Record(std::function<void()> commands);

		RenderPassInlineContent& NextSubPass(std::function<void()> commands);
		void EndPass(std::function<void()> commands);

	private:
		RenderPassInlineContent(VkFramebuffer framebuffer,VkCommandBuffer command_buffer);

		VkFramebuffer	m_Framebuffer;
		VkCommandBuffer m_CommandBuffer;
	};

	class RenderPass {
		friend class Context;
	public:
		uint32_t					GetAttachmentCount();
		uint32_t					GetSubpassCount();
		VkRenderPass			GetRenderPass();

		void					SetDebugName(const std::string& name);

		RenderPassInlineContent	Begin(VkFramebuffer framebuffer,VkClearValue* clear_values,
			VkRect2D render_area,VkViewport viewport,VkRect2D sissor,VkCommandBuffer command_buffer);

		~RenderPass();
	private:
		RenderPass(VkRenderPass render_pass,VkDevice device,uint32_t subpass_count,
			uint32_t attachment_count);

		VkRenderPass m_Pass;
		VkDevice	 m_Device;

		uint32_t m_SubpassCount;
		uint32_t m_AttachmentCount;
	};
}

struct GvkDescriptorLayoutHint
{
	void AddDescriptorSetLayout(const gvk::ptr<gvk::DescriptorSetLayout>& layout);
	std::vector<gvk::ptr<gvk::DescriptorSetLayout>> precluded_descriptor_layouts;
};

struct GvkGraphicsPipelineCreateInfo {

	GvkGraphicsPipelineCreateInfo() {}
	
	//vertex/geometry shader and task/mesh should not coexist!
	//if task/mesh shader is set,InputAssemblyState,VertexInputState,
	//vertex and geometry shader are ignored
	gvk::ptr<gvk::Shader> vertex_shader;
	gvk::ptr<gvk::Shader> geometry_shader;

	gvk::ptr<gvk::Shader> fragment_shader;

	//vertex/geometry shader and task/mesh should not coexist!
	//if task/mesh shader is set,InputAssemblyState,VertexInputState,
	//vertex and geometry shader are ignored
	gvk::ptr<gvk::Shader> task_shader;
	gvk::ptr<gvk::Shader> mesh_shader;
	
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
		FrameBufferBlendState(const FrameBufferBlendState& state);
		FrameBufferBlendState& operator=(const FrameBufferBlendState& state);

		/// <summary>
		/// Resize the blend states to frame buffer count of blend states
		/// </summary>
		/// <param name="frame_buffer_count"></param>
		void Resize(uint32_t frame_buffer_count);

		/// <summary>
		/// set the blend state at specified location
		/// </summary>
		/// <param name="index">index of blend state</param>
		/// <param name="state">state to set</param>
		void Set(uint32_t index,const BlendState& state);

		std::vector<VkPipelineColorBlendAttachmentState> frame_buffer_states;
		VkPipelineColorBlendStateCreateInfo create_info;
	} frame_buffer_blend_state;

	struct InputAssembly : public VkPipelineInputAssemblyStateCreateInfo {
		//constructor will set usually used parameters automatically
		InputAssembly();
	} input_assembly_state;

	uint32_t	max_bindless_binding_count = 1024;

	/// <summary>
	/// constructor of GvkGraphicsPipelineCreateInfo
	/// </summary>
	/// <param name="vert">the vertex shader of graphics pipeline</param>
	/// <param name="frag">the fragment shader of graphics pipeline</param>
	/// <param name="render_pass">the render pass of graphics pipeline</param>
	/// <param name="subpass_index">the index of subpass in render pass of graphics pipeline</param>
	/// <param name="blend_states">pointer to array of blend states of graphics pipeline,the array size must equal to count of output of fragment shader</param>
	GvkGraphicsPipelineCreateInfo(gvk::ptr<gvk::Shader> vert, gvk::ptr<gvk::Shader> frag, gvk::ptr<gvk::RenderPass> render_pass,
		uint32_t subpass_index, const GvkGraphicsPipelineCreateInfo::BlendState* blend_states = NULL);
	

	GvkDescriptorLayoutHint descriptor_layuot_hint;

	gvk::ptr<gvk::RenderPass>	target_pass;
	uint32_t					subpass_index = 0;
};


struct GvkComputePipelineCreateInfo
{
	gvk::ptr<gvk::Shader>		shader;

	GvkDescriptorLayoutHint		descriptor_layuot_hint;
	uint32_t					max_bindless_binding_count = 1024;
};

struct GvkPipelineCIInitializer
{
	/// <summary>
	/// constructor of GvkGraphicsPipelineCreateInfo
	/// </summary>
	/// <param name="vert">the vertex shader of graphics pipeline</param>
	/// <param name="frag">the fragment shader of graphics pipeline</param>
	/// <param name="render_pass">the render pass of graphics pipeline</param>
	/// <param name="subpass_index">the index of subpass in render pass of graphics pipeline</param>
	/// <param name="blend_states">pointer to array of blend states of graphics pipeline,the array size must equal to count of output of fragment shader</param>
	static GvkGraphicsPipelineCreateInfo vertex(gvk::ptr<gvk::Shader> vert, gvk::ptr<gvk::Shader> frag, gvk::ptr<gvk::RenderPass> render_pass,
		uint32_t subpass_index, const GvkGraphicsPipelineCreateInfo::BlendState* blend_states = NULL)
	{
		return GvkGraphicsPipelineCreateInfo(vert, frag, render_pass, subpass_index, blend_states);
	}

	/// <summary>
	/// constructor of GvkGraphicsPipelineCreateInfo
	/// </summary>
	/// <param name="task">the task shader of graphics pipeline</param>
	/// <param name="mesh">the mesh shader of graphics pipeline</param>
	/// <param name="frag">the fragment shader of graphics pipeline</param>
	/// <param name="render_pass">the render pass of graphics pipeline</param>
	/// <param name="subpass_idx">the index of subpass in render pass of graphics pipeline</param>
	/// <param name="blend_states">pointer to array of blend states of graphics pipeline,the array size must equal to count of output of fragment shader</param>
	static GvkGraphicsPipelineCreateInfo mesh(gvk::ptr<gvk::Shader> task, gvk::ptr<gvk::Shader> mesh, gvk::ptr<gvk::Shader> frag, gvk::ptr<gvk::RenderPass> render_pass,
		uint32_t subpass_idx, const GvkGraphicsPipelineCreateInfo::BlendState* blend_states);


	static GvkComputePipelineCreateInfo compute(gvk::ptr<gvk::Shader> comp);
};



struct GvkRenderPassCreateInfo : public VkRenderPassCreateInfo
{
	GvkRenderPassCreateInfo();
	uint32_t	AddColorAttachment(VkAttachmentDescriptionFlags flag, VkFormat format, VkSampleCountFlagBits sample_counts, VkAttachmentLoadOp load,
		VkAttachmentStoreOp store, VkImageLayout init_layout, VkImageLayout end_layout)
	{
		return AddAttachment(flag, format, sample_counts, load, store, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
			init_layout, end_layout);
	}

	uint32_t	AddAttachment(VkAttachmentDescriptionFlags flag,VkFormat format,
		VkSampleCountFlagBits sample_counts,VkAttachmentLoadOp load,
		VkAttachmentStoreOp store,VkAttachmentLoadOp stencil_load,VkAttachmentStoreOp stencil_store,
		VkImageLayout init_layout,VkImageLayout end_layout);

	uint32_t AddSubpass(VkSubpassDescriptionFlags flags = 0, VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS);
	void	AddSubpassColorAttachment(uint32_t subpass_index,uint32_t attachment_index);
	void	AddSubpassDepthStencilAttachment(uint32_t subpass_index,uint32_t attachment_index);
	void	AddSubpassInputAttachment(uint32_t subpass_index,uint32_t attachment_index,VkImageLayout layout);

	void	AddSubpassDependency(uint32_t                srcSubpass,
								 uint32_t                dstSubpass,
							     VkPipelineStageFlags    srcStageMask,
								 VkPipelineStageFlags    dstStageMask,
								 VkAccessFlags           srcAccessMask,
								 VkAccessFlags           dstAccessMask,
								 VkDependencyFlags       dependencyFlags = 0);

	std::vector<VkSubpassDescription>				m_Subpasses;
	std::vector<VkAttachmentReference>				m_SubpassDepthReference;
	std::vector<std::vector<VkAttachmentReference>>	m_SubpassColorReference;
	std::vector<std::vector<VkAttachmentReference>> m_SubpassInputReference;
	std::vector<VkAttachmentDescription>			m_Attachment;
	std::vector<VkSubpassDependency>				m_Dependencies;
};

class GvkPushConstant 
{
	friend class Pipeline;
public:
	void Update(VkCommandBuffer cmd_buffer,const void* data);
	
	GvkPushConstant(VkPushConstantRange range, VkPipelineLayout layout);
	GvkPushConstant():layout(NULL) {}
private:
	VkPushConstantRange range;
	VkPipelineLayout    layout;
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
		DescriptorAllocator(VkDevice device, bool rtSupport);
		void OnBindlessDescriptorSetDestroy(VkDescriptorSet set);

		opt<ptr<DescriptorSet>> AllocateBindless(ptr<DescriptorSetLayout> layout);

		opt<VkDescriptorPool>	CreatePool();

		//TODO : release the descriptor pools that don't used any more
		std::vector<VkDescriptorPool> m_UsedDescriptorPool;
		VkDescriptorPool			  m_CurrentDescriptorPool;
		//	this allocator is self-adaptive, will change the pool size
		//	according to the last pool
		std::vector<VkDescriptorPoolSize> m_PoolSize;


		std::vector<VkDescriptorPool> m_AllocatedBindlessPool;
		std::vector<VkDescriptorSet>  m_AllocatedBindlessSet;

		uint32_t					  m_MaxSetCount;
		VkDevice					  m_Device;
	};

	class DescriptorSet 
	{
		friend class DescriptorAllocator;
	public:
		/// <summary>
		/// Get native vulkan descriptor set
		/// </summary>
		/// <returns>vulkan descriptor set</returns>
		VkDescriptorSet		GetDescriptorSet();
		
		/// <summary>
		/// Get index of descriptor set
		/// </summary>
		/// <returns></returns>
		uint32_t			GetSetIndex();

		opt<SpvReflectDescriptorBinding*> FindBinding(const char* name);

		void SetDebugName(const std::string& name);

		~DescriptorSet();

	private:
		// DescriptorSet(VkDevice device,VkDescriptorSet set,ptr<DescriptorSetLayout> layout);
		DescriptorSet(VkDevice device, VkDescriptorSet set, ptr<DescriptorSetLayout> layout, DescriptorAllocator* alloc);

		VkDevice							m_Device;
		VkDescriptorSet						m_Set;
		ptr<DescriptorSetLayout>			m_Layout;
		DescriptorAllocator*				m_Alloc;
	};


	class Pipeline {
		friend class Context;
	public:
		opt<ptr<RenderPass>>					GetRenderPass();
		opt<ptr<DescriptorSetLayout>>			GetInternalLayout(uint32_t set,VkShaderStageFlagBits stage = (VkShaderStageFlagBits)0);

		VkPipeline								GetPipeline();
		VkPipelineLayout						GetPipelineLayout();

		VkPipelineBindPoint						GetPipelineBindPoint();

		opt<GvkPushConstant>					GetPushConstantRange(const char* name);

		void									SetDebugName(const std::string& name);
		
		virtual ~Pipeline();
	protected:
		Pipeline(VkPipeline pipeline, VkPipelineLayout layout, const std::vector<ptr<DescriptorSetLayout>>& descriptor_set_layouts, const std::unordered_map<std::string,VkPushConstantRange>& push_constants, 
			ptr<RenderPass> render_pass,uint32_t subpass_index,VkPipelineBindPoint bind_point,VkDevice device);


		VkPipelineBindPoint										m_BindPoint;
		VkDevice												m_Device;
		VkPipeline												m_Pipeline;
		VkPipelineLayout										m_PipelineLayout;
		//descriptor sets not created from layout hints will be created internally
		std::vector<ptr<DescriptorSetLayout>>					m_InternalDescriptorSetLayouts;
		std::unordered_map<std::string, VkPushConstantRange>	m_PushConstants;
		ptr<RenderPass>											m_RenderPass;
		uint32_t													m_SubpassIndex;
	};
	
}

struct GvkDescriptorSetBufferWrite
{
	VkDescriptorSet		desc_set;
	VkDescriptorType	type;
	uint32_t				binding;
	uint32_t				array_index;
	VkBuffer			buffer;
	VkDeviceSize		offset;
	VkDeviceSize		size;
};

struct GvkDescriptorSetImageWrite
{
	VkDescriptorSet  desc_set;
	VkDescriptorType type;
	uint32_t			binding;
	uint32_t			array_index;
	VkSampler		sampler;
	VkImageView		image;
	VkImageLayout	layout;
};

struct GvkDescriptorSetAccelerationStructureWrite
{
	VkAccelerationStructureKHR as;
	VkWriteDescriptorSetAccelerationStructureKHR writeAs;
	uint32_t binding;
	VkDescriptorSet set;
};


struct GvkDescriptorSetWrite
{
	std::vector<GvkDescriptorSetBufferWrite> buffers;
	std::vector<GvkDescriptorSetImageWrite>  images;
	std::vector<GvkDescriptorSetAccelerationStructureWrite>  as;


	GvkDescriptorSetWrite& ImageWrite(gvk::ptr<gvk::DescriptorSet> set,VkDescriptorType descriptor_type, uint32_t binding, VkSampler sampler, VkImageView image_view, VkImageLayout layout, uint32_t array_index = 0);
	GvkDescriptorSetWrite& BufferWrite(gvk::ptr<gvk::DescriptorSet> set,VkDescriptorType descriptor_type, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, uint32_t array_index = 0);

	GvkDescriptorSetWrite& ImageWrite(gvk::ptr<gvk::DescriptorSet> set, const char* name, VkSampler sampler, VkImageView image_view, VkImageLayout layout, uint32_t array_index = 0);
	GvkDescriptorSetWrite& BufferWrite(gvk::ptr<gvk::DescriptorSet> set,const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, uint32_t array_index = 0);

	GvkDescriptorSetWrite& AccelerationStructureWrite(gvk::ptr<gvk::DescriptorSet> set, uint32_t binding, VkAccelerationStructureKHR tlas);
	
	void				   Emit(VkDevice device);
};

//TODO currently we don't support dynamic offset
//we assume count of descriptor set wouldn't greater than 16
struct GvkDescriptorSetBindingUpdate
{
	GvkDescriptorSetBindingUpdate(VkCommandBuffer cmd_buffer, gvk::ptr<gvk::Pipeline> pipeline);

	GvkDescriptorSetBindingUpdate& BindDescriptorSet(gvk::ptr<gvk::DescriptorSet> set);

	void Update();

	gvk::uint32 descriptor_set_count;

	std::array<gvk::ptr<gvk::DescriptorSet>,16>  target_sets;
	VkPipelineLayout layout;
	VkPipelineBindPoint bind_point;
	VkCommandBuffer cmd_buffer;
};