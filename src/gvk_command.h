#pragma once
#include "gvk_common.h"
#include "gvk_pipeline.h"
#include "gvk_resource.h"
#include <functional>

namespace gvk {
	// We don't hide command pool from user because importance of command pool in multi-threading
	class CommandPool {
		friend class Context;
		friend class CommandQueue;
	public:

		/// <summary>
		/// Get the queue family index of the command pool
		/// </summary>
		/// <returns></returns>
		uint32 QueueFamily() const { return m_QueueFamilyIndex; }

		/// <summary>
		/// Create a command buffer from this command pool
		/// </summary>
		/// <param name="level">The level of the command buffer</param>
		/// <returns>The created command buffer</returns>
		opt<VkCommandBuffer>  CreateCommandBuffer(VkCommandBufferLevel level);

		void	SetDebugName(const std::string& name);

		~CommandPool();
	private:
		CommandPool(VkCommandPool pool, uint32 queue_family,VkDevice device);
		

		VkCommandPool m_CommandPool;
		uint32        m_QueueFamilyIndex;

		VkDevice      m_Device;
	};

	class SemaphoreInfo {
	private:
		friend class CommandQueue;
		friend class Context;
		std::vector<VkSemaphore> wait_semaphores;
		std::vector<VkPipelineStageFlags> wait_semaphore_stages;
		std::vector<VkSemaphore> signal_semaphores;
	public:
		SemaphoreInfo& Wait(VkSemaphore wait_semaphore, VkPipelineStageFlags stage) {
			wait_semaphores.push_back(wait_semaphore);
			wait_semaphore_stages.push_back(stage);
			return *this;
		}
		SemaphoreInfo& Signal(VkSemaphore signal_semaphore) {
			signal_semaphores.push_back(signal_semaphore);
			return *this;
		}
	};

	class CommandQueue {
		friend class Context;
	public:
		/// <summary>
		/// Get the queue family index of this command queue
		/// </summary>
		/// <returns>Queue family index</returns>
		uint32 QueueFamily() const { return m_QueueFamilyIndex; }

		/// <summary>
		/// Submit command buffers to command queue and excute
		/// </summary>
		/// <param name="cmd_buffers">Array of command buffers to submit</param>
		/// <param name="cmd_buffer_count">Count of the command buffers to submit</param>
		/// <param name="info">Semaphores this queue need to wait and signal</param>
		/// <param name="stall_for_device">If this flag is set to true,the host wait for this queue until the excution finishes</param>
		/// <returns>VkResult of the submission</returns>
		VkResult Submit(VkCommandBuffer* cmd_buffers,uint32 cmd_buffer_count,
			const SemaphoreInfo& info, VkFence target_fence,
			bool stall_for_device = false);

		
		/// <summary>
		/// Create a command buffer, record command to it and submit it immediately
		///	This function is handy for command buffers only executed once. e.g.: commands for uploading resources
		/// </summary>
		/// <param name="command">a function record command to command buffer</param>
		/// <param name="info">the semaphores to wait and signal</param>
		/// <param name="stall_for_host">If this flag is set to true,the host wait for this queue until the excution finishes</param>
		/// <returns>VkResult of the operation</returns>
		VkResult SubmitTemporalCommand(std::function<void(VkCommandBuffer)> command,
			const SemaphoreInfo& info, VkFence target_fence,bool stall_for_host);

		void	SetDebugName(const std::string& name);
		
		~CommandQueue();
	private:
		CommandQueue(VkQueue queue,uint32 queue_family,uint32 queue_index,float priority,VkFence fence,VkDevice device);

		//command pool for command buffers only called once
		VkCommandPool m_TemporalBufferPool;

		VkFence m_Fence;
		VkDevice m_Device;

		VkQueue m_CommandQueue;
		uint32  m_QueueFamilyIndex;
		uint32  m_QueueIndex;
		float	m_Priority;

		Context* m_Context;
	};	
}

void GvkBindPipeline(VkCommandBuffer cmd, gvk::ptr<gvk::Pipeline> pipeline);

struct GvkBindVertexIndexBuffers
{
	GvkBindVertexIndexBuffers(VkCommandBuffer cmd);

	GvkBindVertexIndexBuffers& BindVertex(gvk::ptr<gvk::Buffer> vertex,uint32_t  bind,VkDeviceSize offset = 0);

	GvkBindVertexIndexBuffers& BindIndex(gvk::ptr<gvk::Buffer> index,VkIndexType type,VkDeviceSize offset = 0);

	void Emit();
private:
	VkBuffer idx;
	uint32_t idx_offset;
	VkIndexType idx_type;

	std::array<VkBuffer, 8> verts{NULL};
	std::array<VkDeviceSize, 8> offsets{0};
	uint32_t bind_start;

	VkCommandBuffer cmd;
};

struct GvkDebugMarker
{
	GvkDebugMarker(VkCommandBuffer cmd,const char* name);

	GvkDebugMarker(VkCommandBuffer cmd, const char* name,const float* color);

	void End();

	~GvkDebugMarker();
private:
	VkDebugMarkerMarkerInfoEXT InitInfo(const float* color,const char* name);

	VkCommandBuffer cmd;
};