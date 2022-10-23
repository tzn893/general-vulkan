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
	if (std::find_if(target.begin(), target.end(), GvkExpectStrEqualTo(name)) == target.end()) {
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

	static std::vector<const char*> target_extensions[GVK_INSTANCE_EXTENSION_COUNT] = {
			{VK_EXT_DEBUG_UTILS_EXTENSION_NAME},//GVK_INSTANCE_EXTENSION_DEBUG
	};

	static const char* layer_name_table[GVK_LAYER_COUNT] = {
			"VK_LAYER_KHRONOS_validation",
			"VK_LAYER_LUNARG_monitor",//GVK_INSTANCE_EXTENSION_LUNARG_FPS_MONITOR
	};

	bool Context::AddInstanceExtension(GVK_INSTANCE_EXTENSION e_extension) {
		gvk_assert(e_extension < GVK_INSTANCE_EXTENSION_COUNT);

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

	bool Context::InitializeInstance(GvkInstanceCreateInfo& info,std::string* error)
	{
		for (auto layer : info.required_layers) 
		{
			bool res = AddInstanceLayer(layer);
			if (!res) 
			{
				*error = "gvk: fail to enable layer " + std::string(layer_name_table[layer]);
				return false;
			}
		}
		for (auto ext : info.required_instance_extensions) {
			bool res = AddInstanceExtension(ext);
			if (!res)
			{
				std::string ext_names = "";
				for (auto name : target_extensions[ext]) {
					ext_names = ext_names.append(name).append(" ");
				}
				*error = "gvk : fail to enable instance extension " + ext_names;
				return false;
			}
		}
			
		VkInstanceCreateInfo inst_create{};
		inst_create.pApplicationInfo = &m_AppInfo;
		inst_create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		inst_create.ppEnabledLayerNames = m_RequiredInstanceLayers.empty() ? nullptr : m_RequiredInstanceLayers.data();
		inst_create.enabledLayerCount = m_RequiredInstanceLayers.size();
		
		inst_create.enabledExtensionCount = m_RequiredInstanceExtensions.size();
		inst_create.ppEnabledExtensionNames = m_RequiredInstanceExtensions.empty() ? nullptr : m_RequiredInstanceExtensions.data();

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

	void Context::DestroyVkSemaphore(VkSemaphore semaphore)
	{
		gvk_assert(semaphore != NULL);
		vkDestroySemaphore(m_Device, semaphore, nullptr);
	}

	opt<VkFramebuffer> Context::CreateFrameBuffer(ptr<RenderPass> render_pass, const VkImageView* views, uint32 width, uint32 height, uint32 layers /*= 1*/, VkFramebufferCreateFlags create_flags /*= 0*/)
	{
		uint32 attachment_count = render_pass->GetAttachmentCount();

		VkFramebufferCreateInfo info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		info.flags = create_flags;
		info.renderPass = render_pass->GetRenderPass();
		info.width = width;
		info.height = height;
		info.pAttachments = views;

		if (layers != 1) 
		{
			if (layers != attachment_count) return std::nullopt;
			info.layers = layers;
			info.attachmentCount = 1;
		}
		else 
		{
			info.layers = 1;
			info.attachmentCount = attachment_count;
		}

		VkFramebuffer frame_buffer;
		if (vkCreateFramebuffer(m_Device,&info,nullptr,&frame_buffer) != VK_SUCCESS) 
		{
			return std::nullopt;
		}

		return frame_buffer;
	}

	void Context::DestroyFrameBuffer(VkFramebuffer frame_buffer)
	{
		gvk_assert(frame_buffer != NULL);
		vkDestroyFramebuffer(m_Device, frame_buffer, nullptr);
	}


	opt<VkSampler> Context::CreateSampler(VkSamplerCreateInfo& sampler_info)
	{
		VkSampler sampler;
		if (vkCreateSampler(m_Device, &sampler_info, nullptr, &sampler) != VK_SUCCESS) 
		{
			return std::nullopt;
		}
		return sampler;
	}

	void Context::DestroySampler(VkSampler sampler)
	{
		gvk_assert(sampler != NULL);
		vkDestroySampler(m_Device, sampler, nullptr);
	}

	VkDevice Context::GetDevice()
	{
		return m_Device;
	}

	gvk::View<ptr<gvk::Image>> Context::GetBackBuffers()
	{
		return View(m_BackBuffers);
	}

	Context::~Context() {
		m_Window = nullptr;
		m_PresentQueue = nullptr;

		for (auto semaphore : m_ImageAcquireSemaphore) 
		{
			vkDestroySemaphore(m_Device, semaphore, nullptr);
		}
		for (auto& image : m_BackBuffers) 
		{
			image = nullptr;
		}
		for (auto back_buffer : m_BackBuffers) {
			back_buffer = nullptr;
		}
		if (m_SwapChain) {
			vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		}
		if (m_Surface) {
			vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);
		}
		if (m_Allocator != NULL) {
			vmaDestroyAllocator(m_Allocator);
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
	bool Context::InitializeDevice(const GvkDeviceCreateInfo& create, std::string * error) {
		gvk_assert(m_Device == NULL);
		gvk_assert(m_VkInstance != NULL);

		GvkDeviceCreateInfo::QueueRequireInfo require_present_queue{};
		require_present_queue.count = 1;
		require_present_queue.flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		require_present_queue.priority = 1.0f;
		//
		const_cast<GvkDeviceCreateInfo&>(create).required_queues.push_back(require_present_queue);

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

		uint32 dqc_idx = 0;
		for (uint32 qfidx = 0; qfidx < priority_buffer.size(); qfidx++) {
			//if the queue family is empty,remove it from device_queue_create buffers
			if (priority_buffer[qfidx].empty()) {
				device_queue_create.erase(device_queue_create.begin() + dqc_idx);
				continue;
			}
			device_queue_create[dqc_idx].pQueuePriorities = priority_buffer[qfidx].data();
			device_queue_create[dqc_idx].queueCount = priority_buffer[qfidx].size();
			device_queue_create[dqc_idx].queueFamilyIndex = qf_props[qfidx].index;
			dqc_idx++;
		}

		VkDeviceCreateInfo device_create{};
		device_create.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create.pNext = nullptr;
		device_create.queueCreateInfoCount = device_queue_create.size();
		device_create.pQueueCreateInfos = device_queue_create.data();
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

		//initialize memory allocation
		m_Allocator = NULL;
		bool addressable = false;
		for (auto extension : create.required_extensions)
		{
			if (strcmp(extension,"VK_KHR_buffer_device_address") == 0) 
			{
				addressable = true;
				break;
			}
		}

		m_DeviceAddressable = addressable;
		return IntializeMemoryAllocation(addressable, m_AppInfo.apiVersion, error);
	}

	opt<ptr<gvk::Shader>> Context::CompileShader(const char* file, const ShaderMacros& macros, const char** include_directories, uint32 include_directory_count, const char** search_pathes, uint32 search_path_count, std::string* error)
	{
		ptr<Shader> shader;
		if (auto v = Shader::Compile(file,macros,include_directories,
			include_directory_count,search_pathes,search_path_count,
			error); v.has_value()) 
		{
			shader = v.value();
		}
		else 
		{
			return std::nullopt;
		}

		if (!shader->CreateShaderModule(m_Device).has_value()) 
		{
			return std::nullopt;
		}
		return shader;
	}

	opt<ptr<gvk::Shader>> Context::LoadShader(const char* file, const char** search_pathes, uint32 search_path_count, std::string* error)
	{
		ptr<Shader> shader;
		if (auto v = Shader::Load(file,search_pathes,search_path_count,
			error);v.has_value())
		{
			shader = v.value();
		}
		else 
		{
			return std::nullopt;
		}

		if (!shader->CreateShaderModule(m_Device).has_value())
		{
			return std::nullopt;
		}
		return shader;
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

			swap_chain_info.minImageCount = m_BackBufferCount;

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

		if (vkCreateSwapchainKHR(m_Device, &m_SwapChainCreateInfo, nullptr, &m_SwapChain) != VK_SUCCESS) {
			if (error != nullptr) *error = "gvk : fail to create swap chain";
			return false;
		}

		//if the old swap chain is not empty,we need to destroy the image views
		if (old_swap_chain != NULL) {
			for (uint32 i = 0; i < m_BackBuffers.size(); i++) {
				m_BackBuffers[i] = nullptr;
			}
			vkDestroySwapchainKHR(m_Device, old_swap_chain, nullptr);
		}

		//retrieve back buffer and initialize render target views
		uint32 image_count;
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &image_count, nullptr);
		//the count of image get from swap chain must match the count we required
		gvk_assert(image_count == m_BackBufferCount);
		m_BackBuffers.resize(image_count);
		std::vector<VkImage> vk_back_buffers(image_count);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &image_count, vk_back_buffers.data());

		for (uint32 i = 0;i < image_count;i++) 
		{
			GvkImageCreateInfo image_create_info{};
			image_create_info.arrayLayers = 1;
			image_create_info.extent.depth = 1;
			image_create_info.extent.width = m_SwapChainCreateInfo.imageExtent.width;
			image_create_info.extent.height = m_SwapChainCreateInfo.imageExtent.height;
			image_create_info.format = m_SwapChainCreateInfo.imageFormat;
			image_create_info.imageType = VK_IMAGE_TYPE_2D;
			image_create_info.usage = m_SwapChainCreateInfo.imageUsage;
			image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
			image_create_info.mipLevels = 1;
			// This might not be right but we don't care now
			image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;

			m_BackBuffers[i] = ptr<Image>(new Image(vk_back_buffers[i],NULL,NULL,m_Device, image_create_info));
		}
		
		

		m_BackBuffers.resize(image_count);
		//create back buffer image views
		for (uint32 i = 0; i < image_count;i++) {
			m_BackBuffers[i]->CreateView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1,
				0, 1, VK_IMAGE_VIEW_TYPE_2D);
		}
		return true;
	}
	


	opt<std::tuple<ptr<gvk::Image>, VkSemaphore>> Context::AcquireNextImage(int64_t _timeout /*= -1*/,VkFence fence /*= NULL*/) {
		gvk_assert(m_SwapChain != NULL);
		uint32 timeout = _timeout < 0 ? UINT64_MAX : _timeout;
		uint32 image_index;
		if (vkAcquireNextImageKHR(m_Device, m_SwapChain, timeout, m_ImageAcquireSemaphore[m_CurrentFrameIndex],
			fence, &image_index) != VK_SUCCESS) 
		{
			return std::nullopt;
		}
		m_CurrentBackBufferImageIndex = image_index;
		return std::make_tuple(m_BackBuffers[image_index], m_ImageAcquireSemaphore[m_CurrentFrameIndex]);
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
		if (vkCreateFence(m_Device, &fence_info, nullptr, &fence) != VK_SUCCESS) {
			return std::nullopt;
		}
		return fence;
	}


	void Context::DestroyFence(VkFence fence)
	{
		gvk_assert(fence != NULL);
		vkDestroyFence(m_Device, fence, nullptr);
	}

	uint32 Context::CurrentFrameIndex()
	{
		return m_CurrentFrameIndex;
	}
}

GvkDeviceCreateInfo& GvkDeviceCreateInfo::RequireQueue(VkFlags queue_flags, uint32 count, float priority /*= 1.0f*/)
{
	required_queues.push_back({ queue_flags,count,priority });
	return *this;
}

GvkDeviceCreateInfo& GvkDeviceCreateInfo::AddDeviceExtension(GVK_DEVICE_EXTENSION extension)
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
	case GVK_DEVICE_EXTENSION_BUFFER_DEVICE_ADDRESS:
		AddNotRepeatedElement(required_extensions, "VK_KHR_buffer_device_address");
		break;
	default:
		gvk_assert(false);
		break;
	}
	return *this;
}

GvkInstanceCreateInfo& GvkInstanceCreateInfo::AddInstanceExtension(GVK_INSTANCE_EXTENSION ext)
{
	gvk_assert(ext < GVK_INSTANCE_EXTENSION_COUNT);
	if (std::find(required_instance_extensions.begin(), required_instance_extensions.end(), ext) == required_instance_extensions.end()) 
	{
		required_instance_extensions.push_back(ext);
	}
	return *this;
}

GvkInstanceCreateInfo& GvkInstanceCreateInfo::AddLayer(GVK_LAYER layer)
{
	gvk_assert(layer < GVK_LAYER_COUNT);
	if (std::find(required_layers.begin(),required_layers.end(),layer) == required_layers.end()) 
	{
		required_layers.push_back(layer);
	}
	return *this;
}

GvkSamplerCreateInfo::GvkSamplerCreateInfo(VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mode)
{
	sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	pNext = NULL;

	this->magFilter = magFilter;
	this->minFilter = minFilter;
	
	mipmapMode = mode;
	mipLodBias = 0.0f;
	minLod = 0.0f;
	maxLod = 0.0f;

	addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	anisotropyEnable = VK_FALSE;
	maxAnisotropy = 1.0f;

	borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	unnormalizedCoordinates = VK_FALSE;
	compareEnable = VK_FALSE;
	compareOp = VK_COMPARE_OP_ALWAYS;
	flags = 0;
}
