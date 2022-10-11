#pragma once
#include "gvk_common.h"
#include "gvk_window.h"
#include "gvk_command.h"
#include "gvk_resource.h"
#include "gvk_pipeline.h"

struct GVK_VERSION {
	uint32 v0, v1, v2;
};

enum GVK_LAYER {
	GVK_LAYER_DEBUG,
	GVK_LAYER_FPS_MONITOR,
	GVK_LAYER_COUNT
};

enum GVK_INSTANCE_EXTENSION {
	GVK_INSTANCE_EXTENSION_DEBUG,
	GVK_INSTANCE_EXTENSION_COUNT
};

enum GVK_DEVICE_EXTENSION {
	GVK_DEVICE_EXTENSION_SWAP_CHAIN,
	//ray tracing contains 3 extensions 
	//acceleration structure,ray tracing pipeline,deferred host operation
	GVK_DEVICE_EXTENSION_RAYTRACING,
	GVK_DEVICE_EXTENSION_COUNT
};

struct GVK_DEVICE_CREATE_INFO {
	//by default a present queue will be created
	//other queues should be required when creating device
	struct QueueRequireInfo {
		VkFlags flags;
		uint32  count;
		float   priority;
	};
	std::vector<QueueRequireInfo> required_queues;
	GVK_DEVICE_CREATE_INFO& RequireQueue(VkFlags queue_flags, uint32 count, float priority = 1.0f);

	std::vector<const char*> required_extensions;
	GVK_DEVICE_CREATE_INFO& AddDeviceExtension(GVK_DEVICE_EXTENSION extension);

	GVK_DEVICE_CREATE_INFO() {}
};

struct GVK_INSTANCE_CREATE_INFO {
	GVK_INSTANCE_CREATE_INFO& AddInstanceExtension(GVK_INSTANCE_EXTENSION ext);
	std::vector<GVK_INSTANCE_EXTENSION> required_instance_extensions;
	
	GVK_INSTANCE_CREATE_INFO& AddLayer(GVK_LAYER layer);
	std::vector<GVK_LAYER> required_layers;
};

namespace gvk {

	class Context {
		friend class CommandQueue;
	public:
		static opt<ptr<Context>> CreateContext(const char* app_name,GVK_VERSION app_version,
			uint32 api_version,ptr<Window> window, std::string* error);

		/// <summary>
		/// Initialize the instance in the context
		/// This function should be called before any other function call
		/// </summary>
		/// <param name="info">the information adout the instance (enabled extensions etc.)</param>
		/// <param name="error">if the initialization fails. return error message through this parameter</param>
		/// <returns>if the initialization succeed</returns>
		bool InitializeInstance(GVK_INSTANCE_CREATE_INFO& info,std::string* error);
		
		/// <summary>
		/// Initialize the device in the context
		/// A present queue will be created along with the device
		/// </summary>
		/// <param name="create">the info structure necessary for create of device</param>
		/// <param name="error">if the initiailization fails return error message</param>
		/// <returns>if the initialization succeed</returns>
		bool InitializeDevice(const GVK_DEVICE_CREATE_INFO& create, std::string* error);

		/// <summary>
		/// Initialize the allocator for allocating buffer/image's memory
		/// </summary>
		/// <param name="vk_api_version">The current vulkan api version</param>
		/// <returns>true if intialized successfully</returns>
		bool IntializeMemoryAllocation(uint32 vk_api_version, std::string* error);

		/// <summary>
		/// create the swap chain and present queue
		/// </summary>
		/// <param name="window"></param>
		/// <param name="img_format"></param>
		/// <param name="error"></param>
		/// <returns></returns>
		bool CreateSwapChain(Window* window,VkFormat img_format,std::string* error);
		
		/// <summary>
		/// should be called every time window is resized
		/// </summary>
		/// <param name="window"></param>
		/// <param name="error"></param>
		/// <returns></returns>
		bool UpdateSwapChain(Window* window,std::string* error);

		/// <summary>
		/// Create a command queue from pre-required command queues.
		/// Return nullopt if no matching queue is found in pre-required command queues
		/// </summary>
		/// <param name="flags">the flags of the command queue</param>
		/// <param name="priority">the priority of the command queue</param>
		/// <returns>created command queue</returns>
		opt<ptr<CommandQueue>> CreateQueue(VkFlags flags,float priority = 1.0f);

		/// <summary>
		/// Create a command pool for specified command queue.
		/// The command pool created by this function always has VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.
		/// If you want a temporary command pool don't use this function!
		/// </summary>
		/// <param name="queue"></param>
		/// <returns></returns>
		opt<ptr<CommandPool>> CreateCommandPool(const CommandQueue* queue);
		
		/// <summary>
		/// Return the present queue of current context.Should be called after device is initialized successfully
		/// Otherwise we return a nullopt value
		/// </summary>
		/// <returns>the present queue</returns>
		ptr<CommandQueue> PresentQueue() { gvk_assert(m_PresentQueue != NULL); return m_PresentQueue; }

		/// <summary>
		/// Create a semaphore for synchronization between queues
		/// </summary>
		/// <returns>The created semaphore</returns>
		opt<VkSemaphore>  CreateVkSemaphore();

		/// <summary>
		/// Create a fence from current device.Return nullopt if creation fails
		/// </summary>
		/// <param name="flags">the flags for the fence</param>
		/// <returns>the created fence</returns>
		opt<VkFence> CreateFence(VkFenceCreateFlags flags);


		/// <summary>
		/// Get current frame buffer's index
		/// </summary>
		/// <returns>frame buffer's index</returns>
		uint32 CurrentFrameIndex();

		/// <summary>
		/// Acquire the next image from swap chain.The image may not ready at the time
		/// acquire operation is finished so every command queue will use the image should wait for the
		/// semaphore returned by this function
		/// </summary>
		/// <param name="timeout">The timeout time wait for this operation.If timeout is less than 0,host will wait for this forever</param>
		/// <param name="fence">The fence to signal after it finishes</param>
		/// <returns>the acquired back buffer image,the back buffer's view,the semaphore to wait</returns>
		opt<std::tuple<VkImage, VkImageView, VkSemaphore>> AcquireNextImage(int64_t timeout = -1,VkFence fence = NULL);


		/// <summary>
		/// Present a back buffer
		/// </summary>
		/// <param name="semaphore">The semaphore to wait before the presentation</param>
		/// <returns> Return VK_SUCCESS if success, otherwise return corresponding error message</returns>
		VkResult Present(const SemaphoreInfo& semaphore);


		/// <summary>
		/// Create a buffer from global allocator
		/// </summary>
		/// <param name="buffer_usage">the usage of the buffer</param>
		/// <param name="size">the size of the buffer</param>
		/// <param name="write">the host write strategy of the buffer</param>
		/// <returns>the created buffer</returns>
		opt<ptr<Buffer>> CreateBuffer(VkBufferUsageFlags buffer_usage, uint64_t size, GVK_HOST_WRITE_PROPERTY write);

		/// <summary>
		/// Create a image from global allocator
		/// </summary>
		/// <param name="info">the create info of the image</param>
		/// <returns>created iamge</returns>
		opt<ptr<Image>>  CreateImage(const GvkImageCreateInfo& info);


		opt<ptr<GraphicsPipeline>> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);

		/// <summary>
		/// Create a descriptor set layout of a set slot from several shaders.
		/// It is important that the descriptor bindings inside the set in shaders should be compatiable with each other.
		/// </summary>
		/// <param name="target_shaders">the target shaders to create layout from</param>
		/// <param name="target_binding">the target set slot to create descriptor set</param>
		/// <returns>created descriptor set layout</returns>
		opt<ptr<DescriptorSetLayout>> CreateDescriptorSetLayout(const std::vector<ptr<Shader>>& target_shaders,
			uint32 target_set,std::string* error);
	
		~Context();
	private:
		bool AddInstanceExtension(GVK_INSTANCE_EXTENSION);
		bool AddInstanceLayer(GVK_LAYER layer);

		struct QueueInfo {
			VkFlags flags;
			float   priority;

			uint32  family_index;
			uint32  queue_index;
		};

		struct QueueFamilyInfo {
			uint32 index;
			VkQueueFamilyProperties props;
		};

		struct PhysicalDevicePropertiesAndFeatures {
		private:
			VkPhysicalDeviceProperties2 phy_prop_2;
			VkPhysicalDeviceFeatures2 phy_feature_2;

			bool rt_feature_enabled;
			VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_prop;
			VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_feature;

			std::vector<QueueFamilyInfo> queue_family_prop;
			//add next 
			struct Header {
				VkStructureType    sType;
				void* pNext;
			}* prop_header,* feat_header;

			std::vector<VkExtensionProperties> supported_extensions;

		public:
			void RequireRayTracing();

			PhysicalDevicePropertiesAndFeatures();

			void Initialize(VkPhysicalDevice device);

			uint64_t ScoreDevice() const;

			bool ExtensionSupported(const char* name) const;

			const VkPhysicalDeviceProperties& DeviceProperties()  const { return phy_prop_2.properties ; }
			const VkPhysicalDeviceFeatures&	DeviceFeatures()		const { return phy_feature_2.features; }
			const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& RaytracingProperties() const {return raytracing_prop;}
			const VkPhysicalDeviceRayTracingPipelineFeaturesKHR& RayTracingFeatures() const { return raytracing_feature; }
			View<QueueFamilyInfo> QueueFamilyProperties();
		};

		Context();

		std::vector<const char*> m_RequiredInstanceExtensions;
		std::vector<const char*> m_RequiredInstanceLayers;
		std::unordered_set<std::string> m_SupportedInstanceExtensions;
		std::unordered_set<std::string> m_SupportedInstanceLayers;
		VkApplicationInfo m_AppInfo;
		ptr<Window> m_Window;

		VkInstance m_VkInstance = NULL;

		VkDevice  m_Device = NULL;
		VkPhysicalDevice m_PhyDevice = NULL;
		PhysicalDevicePropertiesAndFeatures m_DevicePropertiesFeature;
		
		std::vector<QueueInfo> m_RequiredQueueInfos;

		VkSwapchainCreateInfoKHR m_SwapChainCreateInfo;
		ptr<CommandQueue> m_PresentQueue = NULL;
		uint32 m_PresentQueueFamilyIndex;
		VkSwapchainKHR m_SwapChain = NULL;
		// This semaphore will be created right after the swap chain is created
		// It will be signaled after the image acquired from swap chain is ready 
		std::vector<VkSemaphore>    m_ImageAcquireSemaphore;
		// Will be initialized after swap chain is created 
		// Increment by 1 after a image is presented
		uint32  m_CurrentFrameIndex;
		// Back buffer image index acquired from swap chain
		// Will be utilized when presenting
		uint32 m_CurrentBackBufferImageIndex = UINT32_MAX;
		
		VkSurfaceKHR m_Surface = NULL;
		//TODO : set back buffer count dynamically by surface's capacity
		uint32 m_BackBufferCount = 3;
		std::vector<VkImage> m_BackBuffers;
		std::vector<VkImageView> m_BackBufferViews;

		//allocator for memory allocation
		VmaAllocator m_Allocator;

		opt<uint32> FindSuitableQueueIndex(VkFlags flags,float priority);
		opt<ptr<CommandQueue>> ConsumePrequiredQueue(uint32 idx);
		void		 OnCommandQueueDestroy(CommandQueue* queue);

	};



}
