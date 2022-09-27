#pragma once
#include "gvk_common.h"
#include "gvk_window.h"
#include "gvk_command.h"

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
	vector<QueueRequireInfo> required_queues;
	void RequireQueue(VkFlags queue_flags, uint32 count, float priority = 1.0f);

	vector<const char*> required_extensions;
	void AddDeviceExtension(GVK_DEVICE_EXTENSION extension);

	GVK_DEVICE_CREATE_INFO() {}
};

namespace gvk {

	class Context {
		friend class CommandQueue;
	public:
		static opt<ptr<Context>> CreateContext(const char* app_name,GVK_VERSION app_version,
			uint32 api_version,ptr<Window> window, string* error);
		
		bool AddInstanceExtension(GVK_INSTANCE_EXTENSION);
		bool AddInstanceLayer(GVK_LAYER layer);
		bool InitializeInstance(string* error);
		
		//a present queue will be created by default
		
		/// <summary>
		/// Initialize the device in the context
		/// A present queue will be created along with the device
		/// </summary>
		/// <param name="create">the info structure necessary for create of device</param>
		/// <param name="error">if the initiailization fails return error message</param>
		/// <returns>if the initialization succeed</returns>
		bool InitializeDevice(const GVK_DEVICE_CREATE_INFO& create, string* error);

		/// <summary>
		/// create the swap chain and present queue
		/// </summary>
		/// <param name="window"></param>
		/// <param name="img_format"></param>
		/// <param name="error"></param>
		/// <returns></returns>
		bool CreateSwapChain(Window* window,VkFormat img_format,string* error);
		
		/// <summary>
		/// should be called every time window is resized
		/// </summary>
		/// <param name="window"></param>
		/// <param name="error"></param>
		/// <returns></returns>
		bool UpdateSwapChain(Window* window,string* error);

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
		opt<VkSemaphore>  CreateSemaphore();

		opt<VkFence> CreateFence();

		uint32 CurrentFrameIndex();

		/// <summary>
		/// Acquire the next image from swap chain.The image may not ready at the time
		/// acquire operation is finished so every command queue will use the image should wait for the
		/// semaphore returned by this function
		/// </summary>
		/// <param name="timeout">The timeout time wait for this operation.If timeout is less than 0,host will wait for this forever</param>
		/// <param name="fence">The fence to signal after it finishes</param>
		/// <returns>the acquired back buffer image,the back buffer's view,the semaphore to wait</returns>
		opt<tuple<VkImage, VkImageView, VkSemaphore>> AcquireNextImage(int64_t timeout = -1,VkFence fence = NULL);

		void Present();
	
		~Context();
	private:
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

			vector<QueueFamilyInfo> queue_family_prop;
			//add next 
			struct Header {
				VkStructureType    sType;
				void* pNext;
			}* prop_header,* feat_header;

			vector<VkExtensionProperties> supported_extensions;

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

		vector<const char*> m_RequiredInstanceExtensions;
		vector<const char*> m_RequiredInstanceLayers;
		unordered_set<string> m_SupportedInstanceExtensions;
		unordered_set<string> m_SupportedInstanceLayers;
		VkApplicationInfo m_AppInfo;
		ptr<Window> m_Window;

		VkInstance m_VkInstance = NULL;

		VkDevice  m_Device = NULL;
		VkPhysicalDevice m_PhyDevice = NULL;
		PhysicalDevicePropertiesAndFeatures m_DevicePropertiesFeature;
		
		vector<QueueInfo> m_RequiredQueueInfos;

		VkSwapchainCreateInfoKHR m_SwapChainCreateInfo;
		ptr<CommandQueue> m_PresentQueue = NULL;
		uint32 m_PresentQueueFamilyIndex;
		VkSwapchainKHR m_SwapChain = NULL;
		// This semaphore will be created right after the swap chain is created
		// It will be signaled after the image acquired from swap chain is ready 
		vector<VkSemaphore>    m_ImageAcquireSemaphore;
		// Will be initialized after swap chain is created 
		// Increment by 1 after a image is presented
		uint32  m_CurrentFrameIndex;
		
		VkSurfaceKHR m_Surface = NULL;
		//TODO : set back buffer count dynamically by surface's capacity
		uint32 m_BackBufferCount = 3;
		vector<VkImage> m_BackBuffers;
		vector<VkImageView> m_BackBufferViews;

		opt<uint32> FindSuitableQueueIndex(VkFlags flags,float priority);
		opt<ptr<CommandQueue>> ConsumePrequiredQueue(uint32 idx);
		void		 OnCommandQueueDestroy(CommandQueue* queue);

	};



}
