#include "gvk_context.h"

struct GvkExpectStrEqualTo {
	GvkExpectStrEqualTo(const char* lhs) {
		this->lhs = lhs;
	}

	bool operator()(const char* rhs) {
		return strcmp(lhs, rhs) == 0;
	}

	const char* lhs;
};


static inline void AddNotRepeatedElement(std::vector<const char*>& target, const char* name) {
	if (std::find_if(target.begin(), target.end(), GvkExpectStrEqualTo(name)) != target.end()) {
		target.push_back(name);
	}
}

namespace gvk {
	opt<ptr<gvk::Context>> Context::CreateContext(const char* app_name, GVK_VERSION app_version,
		uint32 api_version, ptr<Window> window, std::string* error) {
		gvk_assert(window != nullptr);

		ptr<Context> context(new Context());
		context->m_AppInfo.pApplicationName = app_name;
		context->m_AppInfo.applicationVersion = VK_MAKE_VERSION(app_version.v0, app_version.v1, app_version.v2);
		context->m_AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		context->m_AppInfo.pEngineName = nullptr;
		context->m_AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		context->m_AppInfo.apiVersion = api_version;

		std::vector<VkExtensionProperties> ext_props;
		//find all supported extensions
		{
			uint32 ext_count;
			vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
			ext_props.resize(ext_count);
			vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, ext_props.data());

			for (auto& ext : ext_props) {
				context->m_SupportedInstanceExtensions.insert(ext.extensionName);
			}
		}

		std::vector<VkLayerProperties> lay_props;
		{
			uint32 lay_count;
			vkEnumerateInstanceLayerProperties(&lay_count, nullptr);
			lay_props.resize(lay_count);
			vkEnumerateInstanceLayerProperties(&lay_count, lay_props.data());

			for (auto& lay : lay_props) {
				context->m_SupportedInstanceLayers.insert(lay.layerName);
			}
		}

		context->m_Window = window;
		//add glfw required extensions
		{
			uint32 glfw_ext_count;
			auto glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
			for (uint32 i = 0; i < glfw_ext_count; i++) {
				if (!context->m_SupportedInstanceExtensions.count(glfw_exts[i])) {
					if (error) *error = "gvk : glfw required extension " + (std::string)glfw_exts[i] + " is not supported";
					return std::nullopt;
				}
				context->m_RequiredInstanceExtensions.push_back(glfw_exts[i]);
			}
		}

		return context;
	}

	bool Context::AddInstanceExtension(GVK_INSTANCE_EXTENSION e_extension) {
		gvk_assert(e_extension < GVK_INSTANCE_EXTENSION_COUNT);

		static std::vector<const char*> target_extensions[GVK_INSTANCE_EXTENSION_COUNT] = {
			{VK_EXT_DEBUG_UTILS_EXTENSION_NAME},//GVK_INSTANCE_EXTENSION_DEBUG
		};

		std::vector<const char*>& target = target_extensions[e_extension];
		//for removing added extensions
		for(auto extension : target) {
			//extension is not supported
			if (!m_SupportedInstanceExtensions.count(extension)) {
				return false;
			}
		}
		m_RequiredInstanceExtensions.insert(m_RequiredInstanceExtensions.end(), target.begin(), target.end());

		return true;
	}



	bool Context::AddInstanceLayer(GVK_LAYER layer) {
		static const char* layer_name_table[GVK_LAYER_COUNT] = {
			"VK_LAYER_KHRONOS_validation",
			"VK_LAYER_LUNARG_monitor",//GVK_INSTANCE_EXTENSION_LUNARG_FPS_MONITOR
		};
		gvk_assert(layer < GVK_LAYER_COUNT);
		const char* layer_name = layer_name_table[layer];
		if (std::find_if(m_RequiredInstanceLayers.begin(), m_RequiredInstanceLayers.end(),
			GvkExpectStrEqualTo(layer_name))  == m_RequiredInstanceLayers.end())
		{
			if (!m_SupportedInstanceLayers.count(layer_name)) {
				return false;
			}
			m_RequiredInstanceLayers.push_back(layer_name);
		}
		return true;
	}

	bool Context::InitializeInstance(std::string* error)
{
		VkInstanceCreateInfo inst_create{};
		inst_create.pApplicationInfo = &m_AppInfo;
		inst_create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		inst_create.ppEnabledLayerNames = m_RequiredInstanceLayers.empty() ? nullptr : m_RequiredInstanceLayers.data();
		inst_create.enabledLayerCount = m_RequiredInstanceLayers.size();
		
		inst_create.enabledExtensionCount = m_RequiredInstanceExtensions.size();
		inst_create.ppEnabledLayerNames = m_RequiredInstanceExtensions.empty() ? nullptr : m_RequiredInstanceExtensions.data();

		inst_create.pNext = nullptr;

		if (vkCreateInstance(&inst_create,nullptr,&m_VkInstance) != VK_SUCCESS) {
			//TODO log error message
			if (error != nullptr) *error = "gvk : fail to create vulkan instance";
			return false;
		}
		
		return true;
	}

	Context::Context() {
		m_VkInstance = NULL;
		m_Device = NULL;
		m_PhyDevice = NULL;
		memset(&m_AppInfo, 0, sizeof(m_AppInfo));
	}

	opt<uint32> Context::FindSuitableQueueIndex(VkFlags flags, float priority)
	{
		uint32 queue_idx = 0;
		for (auto queue : m_RequiredQueueInfos) {
			if (queue.flags == flags && queue.priority == priority) {
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
		if (auto v = CreateFence(VK_FENCE_CREATE_SIGNALED_BIT);v.has_value()) {
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

	opt<VkSemaphore> Context::CreateVkSemaphore()
	{
		VkSemaphoreCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		VkSemaphore semaphore;
		if (vkCreateSemaphore(m_Device, &info, nullptr, &semaphore) != VK_SUCCESS) 
		{
			return std::nullopt;
		}
		return semaphore;
	}

	Context::~Context() {
		for (auto image_view : m_BackBufferViews) {
			if (image_view != NULL) {
				vkDestroyImageView(m_Device, image_view, nullptr);
			}
		}
		if (m_SwapChain) {
			vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		}
		if (m_Surface) {
			vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);
		}
		if (m_Device != NULL) {
			vkDestroyDevice(m_Device, nullptr);
		}
		if (m_VkInstance != NULL) {
			//physics device will be destroyed at the same time 
			vkDestroyInstance(m_VkInstance, nullptr);
		}
	}

	

	//this function should not be called more than once
	bool Context::InitializeDevice(const GVK_DEVICE_CREATE_INFO& create, std::string * error) {
		gvk_assert(m_Device == NULL);
		gvk_assert(m_VkInstance != NULL);

		GVK_DEVICE_CREATE_INFO::QueueRequireInfo require_present_queue{};
		require_present_queue.count = 1;
		require_present_queue.flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		require_present_queue.priority = 1.0f;
		//
		const_cast<GVK_DEVICE_CREATE_INFO&>(create).required_queues.push_back(require_present_queue);

		//find all physical device
		uint32 phy_device_count;
		vkEnumeratePhysicalDevices(m_VkInstance, &phy_device_count, nullptr);
		std::vector<VkPhysicalDevice> phy_devices(phy_device_count);
		vkEnumeratePhysicalDevices(m_VkInstance, &phy_device_count, phy_devices.data());

		//record physical device's score,pick the device with highest score
		uint32 curr_device_score = 0;
		
		//currently chosen device's queue create info
		std::vector<VkDeviceQueueCreateInfo> curr_queue_create;
		
		for (auto phy_device : phy_devices) {
			PhysicalDevicePropertiesAndFeatures device_prop_feat;

			//ray tracing extension is enabled
			bool ray_tracing_enabled = std::find_if(create.required_extensions.begin(), create.required_extensions.end(),
				GvkExpectStrEqualTo(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) != create.required_extensions.end();
			if (ray_tracing_enabled) {
				device_prop_feat.RequireRayTracing();
			}
			
			device_prop_feat.Initialize(phy_device);
			//we only create device from discrete gpu
			if (device_prop_feat.DeviceProperties().deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				continue;
			}
			//if the device support all the required extension
			for (auto ext : create.required_extensions) {
				if (!device_prop_feat.ExtensionSupported(ext)) {
					//skip the device if the extension is not supported
					continue;
				}
			}

			//ray tracing feature is required but this device doesn't support
			if (ray_tracing_enabled) {
				if (!device_prop_feat.RayTracingFeatures().rayTracingPipeline)
					continue;
			}
			
			//try to create command queues
			auto queue_family_props = device_prop_feat.QueueFamilyProperties();
			std::vector<uint32> required_queue_count_every_family(queue_family_props.size(),0);
			bool queue_req_satisfied = false;
			for (auto& req : create.required_queues) {
				queue_req_satisfied = false;
				for (uint32 i = 0; i < queue_family_props.size(); i++) {
					if ((queue_family_props[i].props.queueFlags & req.flags) == req.flags) {
						uint32 new_requirement =  required_queue_count_every_family[i] + req.count;
						//the queue count of this queue family satisfies this requirement
						if (new_requirement <= queue_family_props[i].props.queueCount) {
							queue_req_satisfied = true;
							required_queue_count_every_family[i] = new_requirement;
							break;
						}
					}
				}
				if (!queue_req_satisfied) {
					break;
				}
			}
			//this device doesn't match the command queue requirements
			if (!queue_req_satisfied) {
				continue;
			}

			//pick the device with highest score
			uint32 device_score = device_prop_feat.ScoreDevice();
			if (device_score > curr_device_score) {
				curr_device_score = device_score;
				m_PhyDevice = phy_device;
				m_DevicePropertiesFeature = device_prop_feat;
			}
		}

		if (m_PhyDevice == NULL) {
			if (error != nullptr) *error = "no compatible physical device";
		}

		//create command queues from device
		auto qf_props = m_DevicePropertiesFeature.QueueFamilyProperties();
		std::vector<VkDeviceQueueCreateInfo> device_queue_create(qf_props.size(), 
			VkDeviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,NULL}
		);
		std::vector<std::vector<float>>			priority_buffer(qf_props.size());
		for (auto& req : create.required_queues) {
			for (uint32 qfidx = 0; qfidx < qf_props.size();qfidx++) {
				if ((req.flags & qf_props[qfidx].props.queueFlags) == req.flags) {
					QueueInfo info;
					//the queue families are sorted.the index member stores their true index
					info.family_index = qf_props[qfidx].index;
					info.flags = req.flags;
					info.priority = req.priority;
					for (uint32 i = 0; i < req.count;i++) {
						//the index of new queue equals to count of queues before it
						//(the same as the current count of priorities of the queue family)
						info.queue_index = priority_buffer[qfidx].size();
						m_RequiredQueueInfos.push_back(info);
						priority_buffer[qfidx].push_back(req.priority);
					}
					break;
				}
			}
		}
		for (uint32 qfidx = 0; qfidx < qf_props.size();) {
			//if the queue family is empty,remove it from device_queue_create buffers
			if (priority_buffer[qfidx].empty() == 0) {
				device_queue_create.erase(device_queue_create.begin() + qfidx);
				continue;
			}
			device_queue_create[qfidx].pQueuePriorities = priority_buffer[qfidx].data();
			device_queue_create[qfidx].queueCount = priority_buffer[qfidx].size();
			device_queue_create[qfidx].queueFamilyIndex = qf_props[qfidx].index;
			qfidx++;
		}

		VkDeviceCreateInfo device_create{};
		device_create.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create.pNext = nullptr;
		device_create.queueCreateInfoCount = device_queue_create.size();
		device_create.pQueueCreateInfos = device_queue_create.data();
		device_create.enabledLayerCount = m_RequiredInstanceLayers.size();
		//device_create.ppEnabledLayerNames is ignored https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#extendingvulkan-layers-devicelayerdeprecation.
		//device_create.enabledLayerCount is deprecated and ignored.
		device_create.enabledExtensionCount = create.required_extensions.size();
		device_create.ppEnabledExtensionNames = create.required_extensions.data();
		device_create.pEnabledFeatures = nullptr;

		if (vkCreateDevice(m_PhyDevice, &device_create, nullptr, &m_Device) != VK_SUCCESS) {
			if (error != nullptr) *error = "gvk : fail to create device";
			return false;
		}


		if (auto present_queue_idx = FindSuitableQueueIndex(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT
			, 1.0f);present_queue_idx.has_value()) {
			uint32 queue_idx = present_queue_idx.value();
			auto& info = m_RequiredQueueInfos[present_queue_idx.value()];
			m_PresentQueueFamilyIndex = info.family_index;
			if (auto present_queue = ConsumePrequiredQueue(queue_idx);present_queue.has_value()) 
			{
				m_PresentQueue = present_queue.value();
			}
			else {
				if (error != nullptr) *error = "fail to get present queue (family index " + std::to_string(info.family_index) + "," +
					", queue index " + std::to_string(info.queue_index) + ")";
				return false;
			}
		}
		else {
			if (error != nullptr) *error = "fail to create present queue for device";
			return false;
		}

		return true;
	}

	Context::PhysicalDevicePropertiesAndFeatures::PhysicalDevicePropertiesAndFeatures() {
		gvk_zero_mem(phy_prop_2);
		gvk_zero_mem(phy_feature_2);

		gvk_zero_mem(raytracing_feature);
		gvk_zero_mem(raytracing_prop);

		phy_prop_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		phy_feature_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		raytracing_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		raytracing_prop.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

		prop_header = (Header*)&phy_prop_2;
		feat_header = (Header*)&phy_feature_2;
	}

	void Context::PhysicalDevicePropertiesAndFeatures::Initialize(VkPhysicalDevice device) {
		vkGetPhysicalDeviceProperties2(device, &phy_prop_2);
		vkGetPhysicalDeviceFeatures2(device, &phy_feature_2);

		//extension properties of the device
		uint32 extension_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
		supported_extensions.resize(extension_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, supported_extensions.data());
	
		//queue family properties of the device
		uint32 qf_count;
		std::vector<VkQueueFamilyProperties> qfps;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &qf_count, nullptr);
		qfps.resize(qf_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &qf_count, qfps.data());
		for (uint32 i = 0; i < qf_count;i++) {
			queue_family_prop.push_back({ i,qfps[i] });
		}

		//scoring queue families, family with more flag bits score higher.
		auto queue_score = [](const QueueFamilyInfo& qfp) {
			//scan every bits 
			uint32 score = 0;
			for (uint32 i = 0; i < sizeof(qfp.props.queueFlags) * 8; i++) {
				score += (qfp.props.queueFlags & (1 << i)) != 0 ? 1 : 0;
			}
			return score;
		};
		//queue family with lesser flag will be listed at front
		std::sort(queue_family_prop.begin(), queue_family_prop.end(), 
		[queue_score]( const QueueFamilyInfo& lhs,const QueueFamilyInfo& rhs) 
			{
				return queue_score(lhs) < queue_score(rhs);
			});
	}

	void Context::PhysicalDevicePropertiesAndFeatures::RequireRayTracing() {
		prop_header->pNext = &raytracing_prop;
		feat_header->pNext = &raytracing_feature;

		prop_header = (Header*)&raytracing_prop;
		feat_header = (Header*)&raytracing_feature;
	}

	uint64_t Context::PhysicalDevicePropertiesAndFeatures::ScoreDevice() const {
		uint64_t score = 0;
		const VkPhysicalDeviceProperties& prop = DeviceProperties();
		score += prop.limits.maxImageDimension2D;
		return score;
	}

	bool Context::PhysicalDevicePropertiesAndFeatures::ExtensionSupported(const char* name) const {
		return std::find_if(supported_extensions.begin(), supported_extensions.end(),
			[&](const VkExtensionProperties& prop) {
				return strcmp(name, prop.extensionName) == 0;
			}) != supported_extensions.end();
	}

	View<Context::QueueFamilyInfo> Context::PhysicalDevicePropertiesAndFeatures::QueueFamilyProperties() {
		return View<QueueFamilyInfo>(queue_family_prop);
	}

	opt<ptr<CommandQueue>> Context::CreateQueue(VkFlags flags, float priority) {
		uint32 queue_idx;
		if (auto v = FindSuitableQueueIndex(flags, priority);v.has_value()) {
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

		return ptr<CommandPool>(new CommandPool(cmd_pool, queue->QueueFamily(),m_Device));
	}

	bool Context::CreateSwapChain(Window* window,VkFormat rt_format,std::string* error) {
		gvk_assert(window != NULL);
		gvk_assert(m_SwapChain == NULL);

#ifdef GVK_WINDOWS_PLATFORM
		//create windows surface on windows platform
		{
			VkWin32SurfaceCreateInfoKHR win_surface{};
			win_surface.hwnd = glfwGetWin32Window(window->GetWindow());
			//get instance handle of this application
			win_surface.hinstance = GetModuleHandle(NULL);
			win_surface.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;

			VkResult res = vkCreateWin32SurfaceKHR(m_VkInstance, &win_surface, nullptr, &m_Surface);
			if (res != VK_SUCCESS) {
				if (error != nullptr) *error = "gvk : fail to create windows surface";
				return false;
			}
		}
#endif
		//create swap chain for the surface
		{
			//get surface's supported formats
			uint32 format_count;
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhyDevice, m_Surface, &format_count, nullptr);
			std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhyDevice, m_Surface, &format_count, surface_formats.data());

			VkColorSpaceKHR rt_color_space = VK_COLOR_SPACE_MAX_ENUM_KHR;
			for (uint32 i = 0; i < surface_formats.size();i++) {
				if (surface_formats[i].format == rt_format) {
					rt_color_space = surface_formats[i].colorSpace;
					break;
				}
			}
			if (rt_color_space == VK_COLOR_SPACE_MAX_ENUM_KHR) {
				if (error != nullptr) *error = "gvk : format is not supported by swap chain";
				return false;
			}

			//check if the swap chain support this present queue
			VkBool32 present_queue_supported;
			vkGetPhysicalDeviceSurfaceSupportKHR(m_PhyDevice, m_PresentQueueFamilyIndex,
				m_Surface,&present_queue_supported);
			if (present_queue_supported != VK_TRUE) {
				if (error != nullptr) *error = "gvk : present queue is not supported";
				return false;
			}

			//these parameters will stay unchanged even if the surface is resized
			VkSwapchainCreateInfoKHR swap_chain_info{};
			swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swap_chain_info.surface = m_Surface;
			//images created by swap chain should have these following flags by default
			swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			swap_chain_info.imageFormat = rt_format;
			swap_chain_info.imageColorSpace = rt_color_space;
			swap_chain_info.imageArrayLayers = 1;

			//swap chain back buffer can only be accessed by present queue
			swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swap_chain_info.queueFamilyIndexCount = 1;
			swap_chain_info.pQueueFamilyIndices = &m_PresentQueueFamilyIndex;

			swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swap_chain_info.clipped = true;
			//TODO: Currently we simply use FIFO present mode may consider other modes
			swap_chain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

			m_SwapChainCreateInfo = swap_chain_info;

			// fail to update the swap chain
			if (!UpdateSwapChain(window,error)) {
				return false;
			}

			//create semaphores for back buffer images
			//one image one semaphore
			m_ImageAcquireSemaphore.resize(m_BackBufferCount);
			for (uint32 i = 0; i != m_BackBufferCount;i++) {
				if (auto s = CreateVkSemaphore();s.has_value()) 
				{
					m_ImageAcquireSemaphore[i] = s.value();
				}
				else 
				{
					if (error != nullptr) *error = "gvk: fail to create semaphore for back buffers";
					return false;
				}
			}
		}

		return true;
	}

	bool Context::UpdateSwapChain(Window* window, std::string* error)
	{
		gvk_assert(m_Surface != NULL);
		// Reset frame index every time swap chain is updated
		m_CurrentFrameIndex = 0;
		m_CurrentBackBufferImageIndex = UINT32_MAX;

		VkSurfaceCapabilitiesKHR surface_capability{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhyDevice, m_Surface, &surface_capability);

		VkExtent2D swap_chain_ext = surface_capability.currentExtent;
		// Width and height are either both -1, or both not -1.
		if (swap_chain_ext.width == (uint32)-1) {
			swap_chain_ext.height = window->GetHeight();
			swap_chain_ext.width = window->GetWidth();
		}

		VkSurfaceTransformFlagBitsKHR swap_chain_transform{};
		if (surface_capability.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
			swap_chain_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else {
			swap_chain_transform = surface_capability.currentTransform;
		}

		VkSwapchainKHR old_swap_chain = m_SwapChain;

		m_SwapChainCreateInfo.oldSwapchain = old_swap_chain;
		m_SwapChainCreateInfo.imageExtent = swap_chain_ext;
		m_SwapChainCreateInfo.preTransform = swap_chain_transform;

		if (!vkCreateSwapchainKHR(m_Device, &m_SwapChainCreateInfo, nullptr, &m_SwapChain)) {
			if (error != nullptr) *error = "gvk : fail to create swap chain";
			return false;
		}

		//retrieve back buffer and initialize render target views
		uint32 image_count;
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &image_count, nullptr);
		//the count of image get from swap chain must match the count we required
		gvk_assert(image_count == m_BackBufferCount);
		m_BackBuffers.resize(image_count);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &image_count, m_BackBuffers.data());
		
		//if the old swap chain is not empty,we need to destroy the image views
		if (old_swap_chain != NULL) {
			for (uint32 i = 0; i < m_BackBufferViews.size();i++) {
				vkDestroyImageView(m_Device, m_BackBufferViews[i], nullptr);
			}
			vkDestroySwapchainKHR(m_Device, old_swap_chain, nullptr);
		}

		m_BackBufferViews.resize(image_count);
		//create back buffer image views
		for (uint32 i = 0; i < image_count;i++) {
			VkImageViewCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			info.flags = 0;
			info.format = m_SwapChainCreateInfo.imageFormat;
			info.image  = m_BackBuffers[i];
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			//we only have 1 image in array,1 mip level
			info.subresourceRange.baseArrayLayer = 0;
			info.subresourceRange.baseMipLevel = 0;
			info.subresourceRange.layerCount = 1;
			info.subresourceRange.levelCount = 1;
			info.viewType = VK_IMAGE_VIEW_TYPE_2D;

			vkCreateImageView(m_Device, &info, nullptr, &m_BackBufferViews[i]);
		}

		return true;
	}
	


	opt<std::tuple<VkImage, VkImageView, VkSemaphore>> Context::AcquireNextImage(int64_t _timeout,VkFence fence) {
		gvk_assert(m_SwapChain != NULL);
		uint32 timeout = _timeout < 0 ? UINT64_MAX : _timeout;
		uint32 image_index;
		if (vkAcquireNextImageKHR(m_Device, m_SwapChain, timeout, m_ImageAcquireSemaphore[m_BackBufferCount],
			fence, &image_index) != VK_SUCCESS) 
		{
			return std::nullopt;
		}
		m_CurrentBackBufferImageIndex = image_index;
		return std::make_tuple(m_BackBuffers[image_index],m_BackBufferViews[image_index],m_ImageAcquireSemaphore[m_CurrentFrameIndex]);
	}

	VkResult Context::Present(const SemaphoreInfo& semaphore)
	{
		// If the back buffer image index is greater than the back buffer count
		// Which means that the swap chain is presented without acquiring a image before
		// This should not happen !
		gvk_assert(m_CurrentBackBufferImageIndex < m_BackBufferCount);

		VkResult vkrs;

		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pNext = nullptr;
		present_info.pSwapchains = &m_SwapChain;
		present_info.swapchainCount = 1;
		present_info.pImageIndices = &m_CurrentBackBufferImageIndex;
		present_info.pWaitSemaphores = semaphore.wait_semaphores.data();
		present_info.waitSemaphoreCount = semaphore.wait_semaphores.size();
		present_info.pResults = &vkrs;
		
		VkResult present_rs = vkQueuePresentKHR(m_PresentQueue->m_CommandQueue, &present_info);

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_BackBufferCount;
		if (vkrs != VK_SUCCESS) return vkrs;
		return present_rs;
	}

	opt<VkFence> Context::CreateFence(VkFenceCreateFlags flags)
	{
		VkFenceCreateInfo fence_info{};
		fence_info.pNext = nullptr;
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = flags;
		VkFence fence;
		if (!vkCreateFence(m_Device, &fence_info, nullptr, &fence) != VK_SUCCESS) {
			return std::nullopt;
		}
		return fence;
	}

	uint32 Context::CurrentFrameIndex()
	{
		return m_CurrentFrameIndex;
	}
}

void GVK_DEVICE_CREATE_INFO::RequireQueue(VkFlags queue_flags, uint32 count, float priority /*= 1.0f*/)
{
	required_queues.push_back({ queue_flags,count,priority });
}

void GVK_DEVICE_CREATE_INFO::AddDeviceExtension(GVK_DEVICE_EXTENSION extension)
{
	switch (extension) {
	case GVK_DEVICE_EXTENSION_RAYTRACING: 
		AddNotRepeatedElement(required_extensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		AddNotRepeatedElement(required_extensions, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		AddNotRepeatedElement(required_extensions, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		break;
	case GVK_DEVICE_EXTENSION_SWAP_CHAIN:
		AddNotRepeatedElement(required_extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		break;
	default:
		gvk_assert(false);
		break;
	}
}
