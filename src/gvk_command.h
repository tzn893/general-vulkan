#pragma once
#include "gvk_common.h"
#include <functional>

namespace gvk {
	class CommandPool {
		friend class Context;
		friend class CommandQueue;
	public:
		uint32 QueueFamily() const { return m_QueueFamilyIndex; }

		opt<VkCommandBuffer>  CreateCommandBuffer(VkCommandBufferLevel level);

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
		uint32 QueueFamily() const { return m_QueueFamilyIndex; }

		VkResult Submit(VkCommandBuffer* cmd_buffers,uint32 cmd_buffer_count,
			const SemaphoreInfo& info,
			bool stall_for_host = false);

		VkResult StallForHost(int64_t timeout = -1);
		
		//this function is handy for command buffers only excuted once. e.g.: commands for uploading resources
		VkResult SubmitTemporalCommand(std::function<void(VkCommandBuffer)> command,
			const SemaphoreInfo& info,bool stall_for_host);
		
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