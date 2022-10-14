#pragma once
#include "gvk_common.h"
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
			const SemaphoreInfo& info,
			bool stall_for_device = false);

		/// <summary>
		/// Host will wait for the queue finishes execution or time out. 
		/// </summary>
		/// <param name="timeout">The time out time.Wait forever if the timeout value is negative</param>
		/// <returns>VkResult of the operation</returns>
		VkResult StallForDevice(int64_t timeout = -1);
		
		
		/// <summary>
		/// Create a command buffer, record command to it and submit it immediately
		///	This function is handy for command buffers only excuted once. e.g.: commands for uploading resources
		/// </summary>
		/// <param name="command">a function record command to command buffer</param>
		/// <param name="info">the semaphores to wait and signal</param>
		/// <param name="stall_for_host">If this flag is set to true,the host wait for this queue until the excution finishes</param>
		/// <returns>VkResult of the operation</returns>
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