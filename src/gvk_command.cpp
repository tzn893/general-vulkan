#include "gvk_command.h"
#include "gvk_context.h"

namespace gvk {
	VkResult CommandQueue::Submit(VkCommandBuffer* cmd_buffers, uint32 cmd_buffer_count,
		const QueueSubmitSemaphoreInfo& semaphore_info, bool stall_for_host)
	{
		VkSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pNext = nullptr;
		
		info.commandBufferCount = cmd_buffer_count;
		info.pCommandBuffers = cmd_buffers;
		
		info.pWaitDstStageMask = semaphore_info.wait_semaphore_stages.data();
		info.pSignalSemaphores = semaphore_info.signal_semaphores.data();
		info.signalSemaphoreCount = semaphore_info.signal_semaphores.size();

		info.waitSemaphoreCount = semaphore_info.wait_semaphores.size();
		info.pWaitSemaphores = semaphore_info.wait_semaphores.data();

		VkResult vkrs = vkQueueSubmit(m_CommandQueue, 1, &info, m_Fence);
		if (vkrs != VK_SUCCESS) return vkrs;

		if (stall_for_host) {
			return StallForHost();
		}

		return VK_SUCCESS;
	}

	VkResult CommandQueue::StallForHost(int64_t _timeout)
	{
		uint64_t timeout;
		if (_timeout < 0) 
		{
			timeout = 0xffffffff;
		}
		else 
		{
			timeout = _timeout;
		}
		return vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, timeout);
	}

	VkResult CommandQueue::SubmitTemporalCommand(function<void(VkCommandBuffer)> command, 
		const QueueSubmitSemaphoreInfo& semaphore_info, bool stall_for_host)
	{
		if (m_TemporalBufferPool == NULL) 
		{
			VkCommandPoolCreateInfo cmd_pool_info{};
			cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmd_pool_info.pNext = nullptr;
			cmd_pool_info.queueFamilyIndex = m_QueueFamilyIndex;
			// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will be short-lived,
			// meaning that they will be reset or freed in a relatively short timeframe. 
			cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

			VkResult create_result = vkCreateCommandPool(m_Device, &cmd_pool_info, nullptr, &m_TemporalBufferPool);
			//fail to create command pool for command buffer
			//return result
			if (create_result != VK_SUCCESS) 
			{
				return create_result;
			}
		}

		VkCommandBufferAllocateInfo cmd_buffer_allocate{};
		cmd_buffer_allocate.commandBufferCount = 1;
		cmd_buffer_allocate.commandPool = m_TemporalBufferPool;
		cmd_buffer_allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_buffer_allocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_buffer_allocate.pNext = nullptr;

		VkCommandBuffer cmd_buffer;
		VkResult vkrs = vkAllocateCommandBuffers(m_Device, &cmd_buffer_allocate, &cmd_buffer);
		if (vkrs != VK_SUCCESS) 
		{
			return vkrs;
		}

		//begin recording commands for command buffer
		VkCommandBufferBeginInfo begin_info{};
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkrs = vkBeginCommandBuffer(cmd_buffer, &begin_info);
		if (vkrs != VK_SUCCESS) return vkrs;
		
		//record commands
		command(cmd_buffer);

		vkrs = vkEndCommandBuffer(cmd_buffer);
		if (vkrs != VK_SUCCESS) return vkrs;

		Submit(&cmd_buffer, 1, semaphore_info, stall_for_host);
	}

	CommandQueue::~CommandQueue()
	{
		//collect allocated queue info
		m_Context->OnCommandQueueDestroy(this);
	}

	CommandQueue::CommandQueue(VkQueue queue, uint32 queue_family, uint32 queue_index, float priority, VkFence fence,VkDevice device):
		m_CommandQueue(queue),m_QueueFamilyIndex(queue_family),m_QueueIndex(queue_index),m_Priority(priority),
		m_Fence(fence),m_Device(device) , m_TemporalBufferPool(NULL)
	{}

	opt<VkCommandBuffer> CommandPool::CreateCommandBuffer(VkCommandBufferLevel level)
	{
		VkCommandBufferAllocateInfo info{};
		info.commandBufferCount = 1;
		info.commandPool = m_CommandPool;
		info.pNext = nullptr;
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		VkCommandBuffer cmd_buffer;
		if (vkAllocateCommandBuffers(m_Device, &info, &cmd_buffer) != VK_SUCCESS) {
			//TODO log message
			return nullopt;
		}
		return cmd_buffer;
	}

	CommandPool::~CommandPool()
	{
		if (m_CommandPool != nullptr)
		{
			vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		}
	}

	CommandPool::CommandPool(VkCommandPool pool, uint32 queue_family, VkDevice device):
		m_CommandPool(pool),m_QueueFamilyIndex(queue_family),m_Device(device)
	{}

}