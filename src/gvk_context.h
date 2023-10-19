#pragma once
#include "gvk_common.h"
#include "gvk_window.h"
#include "gvk_command.h"
#include "gvk_resource.h"
#include "gvk_pipeline.h"
#include "gvk_shader.h"

struct GVK_VERSION {
	uint32_t v0, v1, v2;
};

enum GVK_LAYER {
	GVK_LAYER_DEBUG,
	GVK_LAYER_FPS_MONITOR,
	GVK_LAYER_COUNT
};

enum GVK_INSTANCE_EXTENSION
{
	//includes 2 extensions
	//debug utils and debug report
	GVK_INSTANCE_EXTENSION_DEBUG,
	GVK_INSTANCE_EXTENSION_SHADER_PRINT,

	GVK_INSTANCE_EXTENSION_COUNT
};

enum GVK_DEVICE_EXTENSION
{
	GVK_DEVICE_EXTENSION_SWAP_CHAIN,
	//ray tracing contains 3 extensions 
	//acceleration structure,ray tracing pipeline,deferred host operation
	GVK_DEVICE_EXTENSION_RAYTRACING,
	GVK_DEVICE_EXTENSION_FILL_NON_SOLID,
	GVK_DEVICE_EXTENSION_BUFFER_DEVICE_ADDRESS,
	GVK_DEVICE_EXTENSION_GEOMETRY_SHADER,
	GVK_DEVICE_EXTENSION_DEBUG_MARKER,
	GVK_DEVICE_EXTENSION_MESH_SHADER,
	
	GVK_DEVICE_EXTENSION_COUNT
};

struct GvkDeviceCreateInfo 
{
	//by default a present queue will be created
	//other queues should be required when creating device
	struct QueueRequireInfo 
	{
		VkFlags flags;
		uint32_t  count;
		float   priority;
	};
	std::vector<QueueRequireInfo> required_queues;
	GvkDeviceCreateInfo& RequireQueue(VkFlags queue_flags, uint32_t count, float priority = 1.0f);

	std::vector<const char*> required_extensions;
	VkPhysicalDeviceFeatures required_features;
	void* p_ext_features = NULL;
	void** pp_next_feature = &p_ext_features;

	VkPhysicalDeviceMeshShaderFeaturesEXT mesh_features;
	bool mesh_features_added = false;
	
	GvkDeviceCreateInfo& AddDeviceExtension(GVK_DEVICE_EXTENSION extension);

	GvkDeviceCreateInfo() :required_features{} {}
};

struct GvkInstanceCreateInfo 
{
	GvkInstanceCreateInfo& AddInstanceExtension(GVK_INSTANCE_EXTENSION ext);
	std::vector<GVK_INSTANCE_EXTENSION> required_instance_extensions;
	
	GvkInstanceCreateInfo& AddLayer(GVK_LAYER layer);
	std::vector<GVK_LAYER> required_layers;

	PFN_vkDebugReportCallbackEXT custom_debug_callback = NULL;
};

struct GvkSamplerCreateInfo : public VkSamplerCreateInfo 
{
	GvkSamplerCreateInfo(VkFilter magFilter,VkFilter minFilter,VkSamplerMipmapMode mode);
};

namespace gvk 
{
	// debug callback function filter out unnecessary information
	VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_filtered(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*type*/,
		uint64_t object, size_t location, int32_t message_code,
		const char* layer_prefix, const char* message, void* /*user_data*/);

	class Context {
		friend class CommandQueue;
	public:
		static opt<ptr<Context>> CreateContext(const char* app_name,GVK_VERSION app_version,
			uint32_t api_version,ptr<Window> window, std::string* error);

		/// <summary>
		/// Get count of back buffers in swap chain
		/// </summary>
		/// <returns>count of back buffers</returns>
		uint32_t GetBackBufferCount();

		/// <summary>
		/// Initialize the instance in the context
		/// This function should be called before any other function call
		/// </summary>
		/// <param name="info">the information adout the instance (enabled extensions etc.)</param>
		/// <param name="error">if the initialization fails. return error message through this parameter</param>
		/// <returns>if the initialization succeed</returns>
		bool InitializeInstance(GvkInstanceCreateInfo& info,std::string* error);
		
		/// <summary>
		/// Initialize the physical device, device and surface in the context
		/// A present queue will be created along with the device
		/// </summary>
		/// <param name="create">the info structure necessary for create of device</param>
		/// <param name="error">if the initiailization fails return error message</param>
		/// <returns>if the initialization succeed</returns>
		bool InitializeDevice(const GvkDeviceCreateInfo& create, std::string* error);

		/// <summary>
		/// Enumerate all available back buffer formats on this physical device
		/// Is useful when creating swap chain
		/// </summary>
		/// <returns>available back buffer formats for swap chain</returns>
		std::vector<VkFormat> EnumerateAvailableBackbufferFormats();

		/// <summary>
		/// A helper function for EnumerateAvailableBackbufferFormats
		/// If formats in hints are supported, pick the first format in hints supported by surface
		/// ,otherwise pick the first supported back buffer format.
		/// </summary>
		/// <param name="hints">format hints</param>
		/// <returns>supported back buffer format</returns>
		VkFormat			  PickBackbufferFormatByHint(const std::vector<VkFormat>& hints);


		/// <summary>
		/// Create a shader, the binary code will be saved on disk with name "{file}.spv"
		/// </summary>
		/// <param name="file">the source file of the shader</param>
		/// <param name="macros">macros used to compile the shaders</param>
		/// <param name="include_directories">the include directories of the shader</param>
		/// <param name="include_directory_count">count of include directories</param>
		/// <param name="search_pathes">search pathes of the shader</param>
		/// <param name="search_path_count">count of search pathes of the shader</param>
		/// <param name="error">error message generated if compile fails</param>
		/// <returns>compiled shader with shader module created</returns>
		opt<ptr<Shader>> CompileShader(const char* file,
			const ShaderMacros& macros,
			const char** include_directories, uint32 include_directory_count,
			const char** search_pathes, uint32_t search_path_count,
			std::string* error);

		/// <summary>
		/// Load compiled shader binary code from file and create a shader module
		/// </summary>
		/// <param name="file">file name</param>
		/// <param name="search_pathes">search pathes of the file</param>
		/// <param name="search_path_count">count of search pathes</param>
		/// <param name="error">error message</param>
		/// <returns>loaded shader with shader module created</returns>
		opt<ptr<Shader>> LoadShader(const char* file,
			const char** search_pathes, uint32_t search_path_count,
			std::string* error);

		/// <summary>
		/// create the swap chain and present queue
		/// </summary>
		/// <param name="window"></param>
		/// <param name="img_format"></param>
		/// <param name="error"></param>
		/// <returns></returns>
		bool CreateSwapChain(VkFormat img_format,std::string* error);
		
		/// <summary>
		/// should be called every time window is resized
		/// </summary>
		/// <param name="window"></param>
		/// <param name="error"></param>
		/// <returns></returns>
		bool UpdateSwapChain(std::string* error);

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
		/// Destroy the semaphore
		/// </summary>
		/// <param name="semaphore">target semaphore</param>
		void			  DestroyVkSemaphore(VkSemaphore semaphore);

		/// <summary>
		/// Create a fence from current device.Return nullopt if creation fails
		/// </summary>
		/// <param name="flags">the flags for the fence</param>
		/// <returns>the created fence</returns>
		opt<VkFence> CreateFence(VkFenceCreateFlags flags);


		/// <summary>
		/// Destroy the fence
		/// </summary>
		/// <param name="fence">target fence</param>
		void			  DestroyFence(VkFence fence);

		/// <summary>
		/// Get current frame buffer's index
		/// </summary>
		/// <returns>frame buffer's index</returns>
		uint32_t CurrentFrameIndex();

		/// <summary>
		/// Acquire the next image from swap chain.The image may not ready at the time
		/// acquire operation is finished so every command queue will use the image should wait for the
		/// semaphore returned by this function
		/// </summary>
		/// <param name="res">the VkResult of this operation</param>
		/// <param name="timeout">The timeout time wait for this operation.If timeout is less than 0,host will wait for this forever</param>
		/// <param name="fence">The fence to signal after it finishes</param>
		/// <returns>the acquired back buffer image,the back buffer's view,the semaphore to wait and the image's index(for indexing frame buffer array)</returns>
		opt<std::tuple<ptr<Image>, VkSemaphore,uint32>> AcquireNextImage(VkResult* res = NULL,int64_t timeout = -1,VkFence fence = NULL);

		/// <summary>
		/// If a resize event comes, resize swap chain and back buffer then acquire the next image
		/// </summary>
		/// <param name="timeout">The timeout time wait for this operation.If timeout is less than 0,host will wait for this forever</param>
		/// <param name="fence">The fence to signal after it finishes</param>
		/// <returns>the acquired back buffer image,the back buffer's view,the semaphore to wait and the image's index(for indexing frame buffer array)</returns>
		opt<std::tuple<ptr<Image>, VkSemaphore, uint32>> AcquireNextImageAfterResize(std::function<bool(uint32,uint32)> resize_call_back, std::string* error, int64_t timeout = -1, VkFence fence = NULL);


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
		/// <returns>created image</returns>
		opt<ptr<Image>>  CreateImage(const GvkImageCreateInfo& info);

		/// <summary>
		/// Create a graphics pipeline
		/// </summary>
		/// <param name="create_info">the create info of the graphics pipeline</param>
		/// <returns>created graphics pipeline</returns>
		opt<ptr<Pipeline>>	CreateGraphicsPipeline(const GvkGraphicsPipelineCreateInfo& create_info);

		/// <summary>
		/// Create a compute pipeline
		/// </summary>
		/// <param name="create_info">the create info of the compute pipeline</param>
		/// <returns>created compute pipeline</returns>
		opt<ptr<Pipeline>>	CreateComputePipeline(const GvkComputePipelineCreateInfo& create_info);
		
		/// <summary>
		/// Create a render pass
		/// </summary>
		/// <param name="info">the create info of render pass</param>
		/// <returns>created render pass</returns>
		opt<ptr<RenderPass>>		CreateRenderPass(const GvkRenderPassCreateInfo& info);

		/// <summary>
		/// Create a descriptor set layout of a set slot from several shaders.
		/// It is important that the descriptor bindings inside the set in shaders should be compatiable with each other.
		/// </summary>
		/// <param name="target_shaders">the target shaders to create layout from</param>
		/// <param name="target_binding">the target set slot to create descriptor set</param>
		/// <returns>created descriptor set layout</returns>
		opt<ptr<DescriptorSetLayout>> CreateDescriptorSetLayout(const std::vector<ptr<Shader>>& target_shaders,
			uint32_t target_set,std::string* error);

		/// <summary>
		/// Create a descriptor allocator
		/// </summary>
		/// <returns>created descriptor allocator</returns>
		ptr<DescriptorAllocator>	  CreateDescriptorAllocator();

		/// <summary>
		/// Create a frame buffer for render pass
		/// </summary>
		/// <param name="render_pass">target render pass</param>
		/// <param name="views">array of attachments in the frame buffer, count of views in the array should equal to RenderPass::GetAttachmentCount() or 1</param>
		/// <param name="width">the width of image views</param>
		/// <param name="height">the height of image views</param>
		/// <param name="array_index">The count of layers used in frame buffer.Must be 1 when multiple views are used</param>
		/// <param name="create_flags">Flag of creating frame buffer</param>
		/// <returns>created frame buffer</returns>
		opt<VkFramebuffer>			  CreateFrameBuffer(ptr<RenderPass> render_pass,const VkImageView* views,
			uint32_t width,uint32_t height,uint32_t layers = 1, VkFramebufferCreateFlags create_flags = 0);


		/// <summary>
		/// equal to vkWaitForDeviceIdle
		/// </summary>
		void						  WaitForDeviceIdle();

		/// <summary>
		/// destroy the created frame buffer
		/// </summary>
		/// <param name="frame_buffer">frame buffer to destroy</param>
		void						  DestroyFrameBuffer(VkFramebuffer frame_buffer);

		/// <summary>
		/// create a sampler
		/// </summary>
		/// <param name="sampler_info">the create info of the sampler</param>
		/// <returns>created sampler</returns>
		opt<VkSampler>					  CreateSampler(VkSamplerCreateInfo& sampler_info);

		/// <summary>
		/// Destroy the created sampler
		/// </summary>
		/// <param name="sampler">a not null VkSampler</param>
		void						  DestroySampler(VkSampler sampler);

		/// <summary>
		/// Get the device of this context
		/// </summary>
		/// <returns>device</returns>
		VkDevice					  GetDevice();

		/// <summary>
		/// Get the physical device of the context
		/// </summary>
		/// <returns> physical device </returns>
		VkPhysicalDevice			  GetPhysicalDevice();

		/// <summary>
		/// get back buffers
		/// </summary>
		/// <returns>view of back buffer array</returns>
		View<ptr<Image>>			  GetBackBuffers();

		void						  SetDebugNameImageView(VkImageView view,const std::string& name);
		void						  SetDebugNameSampler(VkSampler sampler,const std::string& name);
		void						  SetDebugNameCommandBuffer(VkCommandBuffer cmd,const std::string& name);

		~Context();
	private:
		
		bool IntializeMemoryAllocation(bool addressable, uint32_t vk_api_version, std::string* error);

		bool AddInstanceExtension(GVK_INSTANCE_EXTENSION);
		bool AddInstanceLayer(GVK_LAYER layer);

		struct QueueInfo {
			VkFlags flags;
			float   priority;

			uint32_t  family_index;
			uint32_t  queue_index;
		};

		struct QueueFamilyInfo {
			uint32_t index;
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
		std::vector<VkValidationFeatureEnableEXT> m_validationFeatureEnabled;
		std::vector<VkValidationFeatureDisableEXT> m_validationFeatureDisabled;
		int m_debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
		bool m_debugCallbackCreate = false;
		VkDebugReportCallbackEXT m_debugCallback = NULL;

		VkDevice  m_Device = NULL;
		VkPhysicalDevice m_PhyDevice = NULL;
		PhysicalDevicePropertiesAndFeatures m_DevicePropertiesFeature;
		
		std::vector<QueueInfo> m_RequiredQueueInfos;

		VkSwapchainCreateInfoKHR m_SwapChainCreateInfo;
		ptr<CommandQueue> m_PresentQueue = NULL;
		uint32_t m_PresentQueueFamilyIndex;
		VkSwapchainKHR m_SwapChain = NULL;
		// This semaphore will be created right after the swap chain is created
		// It will be signaled after the image acquired from swap chain is ready 
		std::vector<VkSemaphore>    m_ImageAcquireSemaphore;
		// Will be initialized after swap chain is created 
		// Increment by 1 after a image is presented
		uint32_t  m_CurrentFrameIndex;
		// Back buffer image index acquired from swap chain
		// Will be utilized when presenting
		uint32_t m_CurrentBackBufferImageIndex = UINT32_MAX;
		
		VkSurfaceKHR m_Surface = NULL;
		//TODO : set back buffer count dynamically by surface's capacity
		uint32_t m_BackBufferCount = 3;
		std::vector<ptr<Image>>  m_BackBuffers;

		//allocator for memory allocation
		VmaAllocator m_Allocator;
		bool		 m_DeviceAddressable;

		opt<uint32_t> FindSuitableQueueIndex(VkFlags flags,float priority);
		opt<ptr<CommandQueue>> ConsumePrequiredQueue(uint32_t idx);
		void		 OnCommandQueueDestroy(CommandQueue* queue);

	};
}
