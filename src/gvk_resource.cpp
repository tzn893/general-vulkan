#include "gvk_resource.h"
#include "gvk_context.h"

namespace gvk {
	bool Context::IntializeMemoryAllocation(bool addressable, uint32 vk_api_version, std::string* error)
	{
		VmaVulkanFunctions funcs{};
		funcs.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
		funcs.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;

		VmaAllocatorCreateInfo info{};
		info.device = m_Device;
		//for vkGetBufferDeviceAddress 
		info.flags = addressable ? VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT : 0;
		info.vulkanApiVersion = vk_api_version;
		info.physicalDevice = m_PhyDevice;
		info.pVulkanFunctions = &funcs;
		info.instance = m_VkInstance;

		return vmaCreateAllocator(&info, &m_Allocator) == VK_SUCCESS;
	}

	opt<ptr<Buffer>> Context::CreateBuffer(VkBufferUsageFlags buffer_usage, uint64_t size, GVK_HOST_WRITE_PROPERTY write)
	{
		gvk_assert(m_Allocator != NULL);
		//can't create buffer with device address from a no addressable device
		if ((buffer_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) && !m_DeviceAddressable)
		{
			return std::nullopt;
		}

		VkBufferCreateInfo buffer_create_info{};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.usage = buffer_usage;
		buffer_create_info.size = size;

		VmaAllocationCreateInfo allocation_create_info{};
		allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		switch (write) 
		{
		case GVK_HOST_WRITE_RANDOM:
			// make the memory mappable and will be mapped at the start of the allocation 
			allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		case GVK_HOST_WRITE_SEQUENTIAL:
			// make the memory mappable and will be mapped at the start of the allocation
			// this memory can't be accessed through Map, it can only be written by Write
			allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		}
		VkBuffer buffer;
		VmaAllocation alloc;
		VmaAllocationInfo alloc_info;
		if (vmaCreateBuffer(m_Allocator, &buffer_create_info, &allocation_create_info,
			&buffer, &alloc, &alloc_info) != VK_SUCCESS) {
			return std::nullopt;
		}

		void* mapped_data = nullptr;
		if (write == GVK_HOST_WRITE_SEQUENTIAL || write == GVK_HOST_WRITE_RANDOM) {
			mapped_data = alloc_info.pMappedData;
			gvk_assert(mapped_data != nullptr);
		}

		bool addressable = (buffer_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) ==
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		return ptr<Buffer>(new Buffer(write, buffer, alloc, mapped_data, m_Allocator, size, addressable,m_Device));
	}

	void Buffer::Write(const void* data, uint64_t dst_offset, uint64_t size)
	{
		gvk_assert(dst_offset + size <= m_BufferSize);
		if (m_MapppedData) {
			void* dst = ((uint8*)m_MapppedData) + dst_offset;
			memcpy(dst, data, size);
		}
	}
	
	opt<void*> Buffer::Map()
	{
		if (m_HostWriteProperty == GVK_HOST_WRITE_RANDOM) {
			return m_MapppedData;
		}
		return std::nullopt;
	}
	
	VkBuffer Buffer::GetBuffer()
	{
		return m_Buffer;
	}
	
	VkDeviceAddress Buffer::GetAddress()
	{
		gvk_assert(m_Addressable);
		VkBufferDeviceAddressInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		info.buffer = m_Buffer;
		return vkGetBufferDeviceAddress(m_Device, &info);
	}

	
	uint64_t Buffer::GetSize()
	{
		return m_BufferSize;
	}

	Buffer::~Buffer()
	{
		vmaDestroyBuffer(m_Allocator, m_Buffer, m_Allocation);
	}

	Buffer::Buffer(GVK_HOST_WRITE_PROPERTY write_prop, VkBuffer buffer, VmaAllocation alloc,void* mapped_data,
		VmaAllocator allocator, uint64_t buffer_size,bool addressable,VkDevice device):
		m_MapppedData(mapped_data),m_Allocation(alloc),m_Buffer(buffer),m_HostWriteProperty(write_prop),
		m_Allocator(allocator),m_BufferSize(buffer_size),m_Addressable(addressable),m_Device(device)
	{}



	opt<ptr<Image>>  Context::CreateImage(const GvkImageCreateInfo& info) {
		gvk_assert(m_Allocator != NULL);

		VkImageCreateInfo create_info{};
		create_info.arrayLayers = info.arrayLayers;
		create_info.extent = info.extent;
		create_info.flags = info.flags;
		create_info.format = info.format;
		create_info.imageType = info.imageType;
		create_info.mipLevels = info.mipLevels;
		create_info.initialLayout = info.initialLayout;
		create_info.tiling = info.tiling;
		create_info.samples = info.samples;
		create_info.usage = info.usage;
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		create_info.pNext = NULL;

		VmaAllocationCreateInfo alloc_create_info{};
		alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		//for images we prefer dedicated memory
		alloc_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

		VkImage image;
		VmaAllocation alloc;
		VmaAllocationInfo alloc_info;
		if (vmaCreateImage(m_Allocator, &create_info, &alloc_create_info, &image, &alloc, &alloc_info) != VK_SUCCESS) {
			return std::nullopt;
		}

		return ptr<Image>(new Image(image, alloc, m_Allocator,m_Device,info));
	}

	VkImage Image::GetImage()
	{
		return m_Image;
	}

	opt<VkImageView> Image::CreateView(VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount, 
		uint32_t baseArrayLayer, uint32_t layerCount,VkImageViewType type)
	{
		VkImageAspectFlags flags = GetAllAspects(m_Info.format);
		if ((flags & aspectMask) != aspectMask) 
		{
			//invalid aspect mask
			return std::nullopt;
		}

		GvkImageSubresourceRange range{};
		range.range.aspectMask = aspectMask;
		range.range.baseMipLevel = baseMipLevel;
		range.range.levelCount = levelCount;
		range.range.baseArrayLayer = baseArrayLayer;
		range.range.layerCount = layerCount;
		range.type = type;

		if (auto res = m_ViewTable.find(range);res != m_ViewTable.end()) 
		{
			//return the image view already created
			return res->second;
		}

		VkImageViewCreateInfo image_view_create{};
		image_view_create.subresourceRange = range.range;
		image_view_create.viewType = type;
		image_view_create.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create.pNext = NULL;
		image_view_create.image = m_Image;
		//we only use default component mapping
		image_view_create.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		image_view_create.format = m_Info.format;
		//currently we don't care about density map whatever
		image_view_create.flags = 0;
		
		VkImageView view;
		if (vkCreateImageView(m_Device, &image_view_create, NULL, &view) != VK_SUCCESS) 
		{
			return std::nullopt;
		}

		m_ViewTable[range] = view;
		m_Views.push_back(view);

		return view;
	}

	gvk::View<VkImageView> Image::GetViews()
	{
		return View(m_Views);
	}

	Image::~Image()
	{
		//destroy create image views
		for (auto view : m_ViewTable)
		{
			vkDestroyImageView(m_Device, view.second, nullptr);
		}

		//if the image has a allocator and allocation
		//the image must not be created from vma allocation(e.g. acquired from swap chain)
		//so we do nothing
		if (m_Allocator && m_Allocation) {
			vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
		}
	}

	Image::Image(VkImage image, VmaAllocation alloc, VmaAllocator allocator,VkDevice device, const GvkImageCreateInfo& info):
		m_Image(image),m_Allocation(alloc),m_Allocator(allocator),m_Device(device),m_Info(info)
	{}
}

bool GvkImageSubresourceRange::operator==(const GvkImageSubresourceRange& other) const
{
	return memcmp(&range, &other.range, sizeof(range)) == 0
		&& type == other.type;
}

GvkImageCreateInfo GvkImageCreateInfo::Image2D(VkFormat format, uint32 width, uint32 height,
	VkImageUsageFlags usage)
{
	GvkImageCreateInfo create_info{};
	create_info.arrayLayers = 1;
	create_info.mipLevels = 1;
	create_info.imageType = VK_IMAGE_TYPE_2D;
	create_info.extent.depth = 1;
	create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	create_info.flags = 0;


	create_info.format = format;
	create_info.extent.width = width;
	create_info.extent.height = height;
	create_info.usage = usage;

	return create_info;
}

GvkImageCreateInfo GvkImageCreateInfo::MippedImage2D(VkFormat format, uint32 width, uint32 height, uint32 miplevels, VkImageUsageFlags usages)
{
	GvkImageCreateInfo create_info = Image2D(format, width, height, usages);
	create_info.mipLevels = miplevels;

	return create_info;
}


GvkBarrier& GvkBarrier::ImageBarrier(ptr<gvk::Image> image, VkImageLayout init_layout, VkImageLayout final_layout,
	VkAccessFlags src_access_flags, VkAccessFlags dst_access_flags)
{
	VkImageMemoryBarrier image_memory_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	image_memory_barrier.srcAccessMask = src_access_flags;
	image_memory_barrier.dstAccessMask = dst_access_flags;

	image_memory_barrier.newLayout = final_layout;
	image_memory_barrier.oldLayout = init_layout;

	image_memory_barrier.subresourceRange.aspectMask = gvk::GetAllAspects(image->Info().format);
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = image->Info().arrayLayers;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = image->Info().mipLevels;

	image_memory_barrier.image = image->GetImage();

	this->image_memory_barriers.push_back(image_memory_barrier);

	return *this;
}

GvkBarrier& GvkBarrier::BufferBarrier(ptr<gvk::Buffer> buffer, VkAccessFlags src_access_flags, VkAccessFlags dst_access_flags)
{
	VkBufferMemoryBarrier buffer_memory_barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	buffer_memory_barrier.dstAccessMask = dst_access_flags;
	buffer_memory_barrier.srcAccessMask = src_access_flags;

	buffer_memory_barrier.buffer = buffer->GetBuffer();
	buffer_memory_barrier.offset = 0;
	buffer_memory_barrier.size = buffer->GetSize();

	this->buffer_memory_barriers.push_back(buffer_memory_barrier);
	return *this;
}

GvkBarrier& GvkBarrier::MemoryBarrier(VkAccessFlags src_access_flags, VkAccessFlags dst_access_flags)
{
	VkMemoryBarrier memory_barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
	memory_barrier.dstAccessMask = dst_access_flags;
	memory_barrier.srcAccessMask = src_access_flags;

	memory_barriers.push_back(memory_barrier);
	return *this;
}

void GvkBarrier::Emit(VkCommandBuffer cmd_buffer, VkDependencyFlags flag /*= 0*/)
{
	vkCmdPipelineBarrier(cmd_buffer, src_stage, dst_stage,
		flag,
		memory_barriers.size(),!memory_barriers.empty() ?  memory_barriers.data() : NULL,
		buffer_memory_barriers.size(),!buffer_memory_barriers.empty() ? buffer_memory_barriers.data() : NULL,
		image_memory_barriers.size() ,!image_memory_barriers.empty() ? image_memory_barriers.data() : NULL
	);
}

GvkBarrier::GvkBarrier(VkPipelineStageFlags src, VkPipelineStageFlags dst)
:src_stage(src),dst_stage(dst) 
{}

