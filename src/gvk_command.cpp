#include "gvk_command.h"
#include "gvk_context.h"

namespace gvk {

	opt<uint32> Context::FindSuitableQueueIndex(VkFlags flags, float priority)
	{
		uint32 queue_idx = 0;
		for (auto queue : m_RequiredQueueInfos) {
			if ((queue.flags & flags) == flags && queue.priority == priority) {
				break;
			}
			queue_idx++;
		}
		if (queue_idx >= m_RequiredQueueInfos.size()) {
			return std::nullopt;
		}
		return queue_idx;
	}

	opt<ptr<CommandQueue>> Context::ConsumePrequiredQueue(uint32 idx)
	{
		gvk_assert(idx < m_RequiredQueueInfos.size());
		QueueInfo info = m_RequiredQueueInfos[idx];
		VkQueue queue = NULL;
		vkGetDeviceQueue(m_Device, info.family_index, info.queue_index, &queue);
		if (queue == NULL) return std::nullopt;

		//create a fence for queue for cpu synchronization
		VkFence fence;
		if (auto v = CreateFence(VK_FENCE_CREATE_SIGNALED_BIT); v.has_value()) {
			fence = v.value();
		}
		else {
			return std::nullopt;
		}

		m_RequiredQueueInfos.erase(m_RequiredQueueInfos.begin() + idx);
		ptr<CommandQueue> queue_ptr(new CommandQueue(queue, info.family_index, info.queue_index, info.priority, fence, m_Device));
		queue_ptr->m_Context = this;
		return { queue_ptr };
	}

	opt<ptr<CommandQueue>> Context::CreateQueue(VkFlags flags, float priority) {
		uint32 queue_idx;
		if (auto v = FindSuitableQueueIndex(flags, priority); v.has_value()) {
			queue_idx = v.value();
		}
		else {
			return std::nullopt;
		}

		return ConsumePrequiredQueue(queue_idx);
	}

	opt<ptr<CommandPool>> Context::CreateCommandPool(const CommandQueue* queue)
	{
		VkCommandPoolCreateInfo info{};
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		info.queueFamilyIndex = queue->QueueFamily();
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.pNext = nullptr;
		VkCommandPool cmd_pool;
		if (vkCreateCommandPool(m_Device, &info, nullptr, &cmd_pool) != VK_SUCCESS) {
			return std::nullopt;
		}

		return ptr<CommandPool>(new CommandPool(cmd_pool, queue->QueueFamily(), m_Device));
	}

	void Context::OnCommandQueueDestroy(CommandQueue* queue)
	{
		gvk_assert(queue != nullptr);
		QueueInfo info{};
		info.family_index = queue->m_QueueFamilyIndex;
		info.queue_index = queue->m_QueueIndex;

		//the queue family's supported flags can be found in QueueFamilyInfo
		auto queue_family_props = m_DevicePropertiesFeature.QueueFamilyProperties();
		info.flags = queue_family_props[info.family_index].props.queueFlags;
		info.priority = queue->m_Priority;
		m_RequiredQueueInfos.push_back(info);

		if (queue->m_Fence != nullptr) {
			vkDestroyFence(m_Device, queue->m_Fence, nullptr);
		}
	}


	VkResult CommandQueue::Submit(VkCommandBuffer* cmd_buffers, uint32 cmd_buffer_count,
		const SemaphoreInfo& semaphore, VkFence target_fence, bool stall_for_device /*= false*/)
	{
		gvk_assert(target_fence == NULL || !stall_for_device);
		//reset the command queue's fence before submitting
		if (stall_for_device) 
		{
			vkResetFences(m_Device, 1, &m_Fence);
		}

		VkSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pNext = nullptr;
		
		info.commandBufferCount = cmd_buffer_count;
		info.pCommandBuffers = cmd_buffers;
		
		info.pWaitDstStageMask = semaphore.wait_semaphore_stages.data();
		info.pSignalSemaphores = semaphore.signal_semaphores.data();
		info.signalSemaphoreCount = semaphore.signal_semaphores.size();

		info.waitSemaphoreCount = semaphore.wait_semaphores.size();
		info.pWaitSemaphores = semaphore.wait_semaphores.data();

		
		if (stall_for_device) target_fence = m_Fence;
		VkResult vkrs = vkQueueSubmit(m_CommandQueue, 1, &info, target_fence);
		if (vkrs != VK_SUCCESS) return vkrs;

		if (stall_for_device) 
		{
			return vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, 0xffffffff);
		}

		return VK_SUCCESS;
	}

	

	VkResult CommandQueue::SubmitTemporalCommand(std::function<void(VkCommandBuffer)> command, 
		const SemaphoreInfo& info, VkFence target_fence, bool stall_for_host)
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

		return Submit(&cmd_buffer, 1, info, target_fence, stall_for_host);
	}

	CommandQueue::~CommandQueue()
	{
		//collect allocated queue info
		m_Context->OnCommandQueueDestroy(this);
	}

	CommandQueue::CommandQueue(VkQueue queue, uint32 queue_family, uint32 queue_index, float priority, VkFence fence,VkDevice device):
		m_CommandQueue(queue),m_QueueFamilyIndex(queue_family),m_QueueIndex(queue_index),m_Priority(priority),
		m_Fence(fence),m_Device(device) , m_TemporalBufferPool(NULL)
	{
		m_Context = NULL;
	}

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
			return std::nullopt;
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