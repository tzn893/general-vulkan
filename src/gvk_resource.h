#pragma once
#include <vma/vk_mem_alloc.h>
#include "gvk_common.h"
#include <unordered_map>

enum GVK_HOST_WRITE_PROPERTY {
	GVK_HOST_WRITE_NONE,
	GVK_HOST_WRITE_SEQUENTIAL,
	GVK_HOST_WRITE_RANDOM
};


//It seems that the image sharing mode and queue family indices cloud be ignored
struct GvkImageCreateInfo {
	VkImageCreateFlags       flags;
	VkImageType              imageType;
	VkFormat                 format;
	VkExtent3D               extent;
	uint32_t                 mipLevels;
	uint32_t                 arrayLayers;
	VkSampleCountFlagBits    samples;
	VkImageTiling            tiling;
	VkImageUsageFlags        usage;
	VkImageLayout            initialLayout;
};


struct GvkImageSubresourceRange {

	bool operator==(const GvkImageSubresourceRange& other) const;

	VkImageSubresourceRange range;
	VkImageViewType type;
};

namespace std {
	template <>
	struct hash<GvkImageSubresourceRange> {
		std::size_t operator()(const GvkImageSubresourceRange& k) const
		{
			return static_cast<size_t>(k.range.baseArrayLayer)
				^ (static_cast<size_t>(k.range.layerCount) << 20)
				//mip levels only has 6 bits,this is reasonable for texture has 64 mipmap levels 
				//is nearly impossible to storage
				^ (static_cast<size_t>(k.range.baseMipLevel) << 40)
				^ (static_cast<size_t>(k.range.baseMipLevel) << 46)
				^ (static_cast<size_t>(k.type) << 52)
				^ (static_cast<size_t>(k.range.aspectMask) << 56);
		}
	};
}

//the allocator pool will be hided from users by Vma
namespace gvk {
	class Buffer {
		friend class Context;
	public:
		/// <summary>
		/// Write a chunk of data to some position on the buffer.
		/// Only buffer with write property GVK_HOST_WRITE_SEQUENTIAL or GVK_HOST_WRITE_RANDOM can be written
		/// </summary>
		/// <param name="data">pointer to source data</param>
		/// <param name="dst_offset">destination position's offset from start of the buffer</param>
		/// <param name="size">size of the data to write</param>
		void       Write(const void* data, uint64_t dst_offset, uint64_t size);
		
		/// <summary>
		/// Get the mapped pointer of buffer's data on host.
		/// Only buffer with write property GVK_HOST_WRITE_RANDOM can be mapped
		/// </summary>
		/// <returns>the mapped pointer</returns>
		opt<void*> Map();

		/// <summary>
		/// Get VkBuffer
		/// </summary>
		/// <returns>buffer</returns>
		VkBuffer	    GetBuffer();

		/// <summary>
		/// Get device address of the buffer
		/// Only buffer with usage VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT can get address
		/// </summary>
		/// <returns>the device address</returns>
		VkDeviceAddress GetAddress();

		~Buffer();
	private:
		Buffer(GVK_HOST_WRITE_PROPERTY write_prop, VkBuffer buffer, VmaAllocation alloc,void* mapped_data,
			VmaAllocator allocator,uint64_t buffer_size,bool addressable,VkDevice device);

		VkDevice m_Device;

		GVK_HOST_WRITE_PROPERTY m_HostWriteProperty;
		VkBuffer m_Buffer;
		VmaAllocation m_Allocation;
		VmaAllocator m_Allocator;
		uint64_t m_BufferSize;

		void* m_MapppedData;
		bool  m_Addressable;
	};

	class Image {
		friend class Context;
	public:
		/// <summary>
		/// Get vulkan image handle for the image
		/// </summary>
		/// <returns>vulkan image handle</returns>
		VkImage GetImage();
		
		/// <summary>
		/// Get the image info of the image
		/// </summary>
		/// <returns>the info</returns>
		const GvkImageCreateInfo & Info() { return m_Info; }

		/// <summary>
		/// Create a image view for a subresource range.
		/// Every image view created from image is staged, so you don't have to release it.
		/// If the range matches the range of a staged view, the staged view will be returned
		/// </summary>
		/// <param name="aspectMask">the aspect mask for the view</param>
		/// <param name="baseMipLevel">the start of the mip level of the view</param>
		/// <param name="levelCount">the count of mip level of the view</param>
		/// <param name="baseArrayLayer">the start of texture's array of the view</param>
		/// <param name="layerCount">the count of texture array elements in the view</param>
		/// <param name="type">the type of the image view</param>
		/// <returns>created view</returns>
		opt<VkImageView> CreateView(VkImageAspectFlags    aspectMask,
		uint32_t              baseMipLevel,
		uint32_t              levelCount,
		uint32_t              baseArrayLayer,
		uint32_t              layerCount,
		VkImageViewType       type);

		~Image();
	private:
		Image(VkImage image,VmaAllocation alloc,VmaAllocator allocator,VkDevice device,const GvkImageCreateInfo& info);

		GvkImageCreateInfo m_Info;
		
		VkImage m_Image;
		VmaAllocation m_Allocation;
		VmaAllocator m_Allocator;
		VkDevice m_Device;

		std::unordered_map<GvkImageSubresourceRange, VkImageView> m_CreatedViews;
	};
}
