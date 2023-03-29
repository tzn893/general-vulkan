#include "gvk_command.h"
#include "gvk_context.h"


extern gvk::GvkExtensionFunctionManager g_ExtFunctionManager;

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

	void CommandQueue::SetDebugName(const std::string& name)
	{
		VkDebugMarkerObjectNameInfoEXT info{};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		// Type of the object to be named
		info.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT;
		// Handle of the object cast to unsigned 64-bit integer
		info.object = (uint64_t)m_CommandQueue;
		// Name to be displayed in the offline debugging application
		info.pObjectName = name.c_str();

		g_ExtFunctionManager.vkDebugMarkerSetObjectNameEXT(m_Device, &info);
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

	void CommandPool::SetDebugName(const std::string& name)
	{
		VkDebugMarkerObjectNameInfoEXT info{};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		// Type of the object to be named
		info.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT;
		// Handle of the object cast to unsigned 64-bit integer
		info.object = (uint64_t)m_CommandPool;
		// Name to be displayed in the offline debugging application
		info.pObjectName = name.c_str();

		g_ExtFunctionManager.vkDebugMarkerSetObjectNameEXT(m_Device, &info);
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

void GvkBindPipeline(VkCommandBuffer cmd, gvk::ptr<gvk::Pipeline> pipeline)
{
	vkCmdBindPipeline(cmd, pipeline->GetPipelineBindPoint(), pipeline->GetPipeline());
}

GvkBindVertexIndexBuffers::GvkBindVertexIndexBuffers(VkCommandBuffer cmd)
{
	this->cmd = cmd;
	bind_start = verts.size();
	idx = NULL;

	
}

GvkBindVertexIndexBuffers& GvkBindVertexIndexBuffers::BindVertex(gvk::ptr<gvk::Buffer> vertex, uint32_t bind, VkDeviceSize offset /*= 0*/)
{
	gvk_assert(bind < 8);

	verts[bind] = vertex->GetBuffer();
	offsets[bind] = offset;
	
	if (bind_start > bind) bind_start = bind;
	return *this;
}

GvkBindVertexIndexBuffers& GvkBindVertexIndexBuffers::BindIndex(gvk::ptr<gvk::Buffer> index, VkIndexType type, VkDeviceSize offset /*= 0*/)
{
	idx = index->GetBuffer();
	idx_offset = offset;
	idx_type = type;
	return *this;
}

void GvkBindVertexIndexBuffers::Emit()
{
	if (idx != NULL)
	{
		vkCmdBindIndexBuffer(cmd, idx, idx_offset, idx_type);
	}

	uint32_t last_bind_start = bind_start;
	for (uint32_t i = last_bind_start;i < 8;i++)
	{
		if (verts[i] == NULL)
		{
			if (last_bind_start != i)
			{
				vkCmdBindVertexBuffers(cmd, last_bind_start, i - last_bind_start, verts.data() + last_bind_start,
					offsets.data() + last_bind_start);
			}
			last_bind_start = i + 1;
		}
	}
	if (last_bind_start != 8)
	{
		vkCmdBindVertexBuffers(cmd, last_bind_start, 8 - last_bind_start, verts.data() + last_bind_start,
			offsets.data() + last_bind_start);
	}
}

GvkDebugMarker::GvkDebugMarker(VkCommandBuffer cmd, const char* name)
{
	this->cmd = cmd;
	float color[4] = { 1,1,1,1 };
	VkDebugMarkerMarkerInfoEXT info = InitInfo(color, name);

	g_ExtFunctionManager.vkCmdDebugMarkerBeginEXT(cmd, &info);
}

GvkDebugMarker::GvkDebugMarker(VkCommandBuffer cmd, const char* name,const float* color)
{
	this->cmd = cmd;
	VkDebugMarkerMarkerInfoEXT info = InitInfo(color, name);
	
	g_ExtFunctionManager.vkCmdDebugMarkerBeginEXT(cmd, &info);
}

void GvkDebugMarker::End()
{
	if (cmd != NULL)
	{
		g_ExtFunctionManager.vkCmdDebugMarkerEndEXT(cmd);
		cmd = NULL;
	}
}

GvkDebugMarker::~GvkDebugMarker()
{
	End();
}

VkDebugMarkerMarkerInfoEXT GvkDebugMarker::InitInfo(const float* color, const char* name)
{
	VkDebugMarkerMarkerInfoEXT info;

	info.pNext = NULL;
	info.color[0] = color[0];
	info.color[1] = color[1];
	info.color[2] = color[2];
	info.color[3] = color[3];
	info.pMarkerName = name;
	info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;

	return info;
}
