#include "gvk.h"
using namespace gvk;

#include "timer.h"

#define require(expr,target) if(auto v = expr;v.has_value()) { target = v.value(); } else { gvk_assert(false);return -1; }

int main()
{
	VkFormat back_buffer_format;

	ptr<gvk::Window> window;
	require(gvk::Window::Create(600, 600, "compute"), window);

	std::string error;
	ptr<gvk::Context> context;
	require(gvk::Context::CreateContext("compute", GVK_VERSION{ 1,0,0 }, VK_API_VERSION_1_3, window, &error),
		context);
	
	current_time();

	GvkInstanceCreateInfo instance_create;
	instance_create.AddInstanceExtension(GVK_INSTANCE_EXTENSION_DEBUG);
	instance_create.AddLayer(GVK_LAYER_DEBUG);
	instance_create.AddLayer(GVK_LAYER_FPS_MONITOR);

	context->InitializeInstance(instance_create, &error);

	GvkDeviceCreateInfo device_create;
	device_create.AddDeviceExtension(GVK_DEVICE_EXTENSION_SWAP_CHAIN);
	device_create.RequireQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT, 1);

	context->InitializeDevice(device_create, &error);

	back_buffer_format = context->PickBackbufferFormatByHint({ VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_R8G8B8A8_UNORM });
	context->CreateSwapChain(back_buffer_format, &error);

	const char* include_directorys[] = { COMPUTE_SHADER_DIRECTORY };

	auto comp = context->CompileShader("julia.comp", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error).value();


	//we don't need any render pass
	auto queue = context->CreateQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT).value();
	auto descriptor_allocator = context->CreateDescriptorAllocator();

	GvkComputePipelineCreateInfo compute_pipeline_create{};
	compute_pipeline_create.shader = comp;
	
	auto pipeline = context->CreateComputePipeline(compute_pipeline_create).value();
	auto descriptor_layout = pipeline->GetInternalLayout(0, VK_SHADER_STAGE_COMPUTE_BIT).value();

	uint32 back_buffer_count = context->GetBackBufferCount();
	std::vector<ptr<gvk::DescriptorSet>> descriptor_sets(back_buffer_count);
	for (uint32 i = 0;i < context->GetBackBufferCount();i++)
	{
		auto buffer = context->GetBackBuffers()[i];
		descriptor_sets[i] = descriptor_allocator->Allocate(descriptor_layout).value();
		//general layout for storage image
		GvkDescriptorSetWrite().ImageWrite(descriptor_sets[i], "o_image", NULL, buffer->GetViews()[0],
			VK_IMAGE_LAYOUT_GENERAL).Emit(context->GetDevice());
	}

	ptr<gvk::CommandPool> cmd_pool	= context->CreateCommandPool(queue.get()).value();
	VkCommandBuffer cmd_buffer		= cmd_pool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY).value();

	//push constants
	GvkPushConstant p_time		= pipeline->GetPushConstantRange("p_time").value();
	GvkPushConstant p_resolution	= pipeline->GetPushConstantRange("p_resolution").value();
	GvkPushConstant p_speed		= pipeline->GetPushConstantRange("p_speed").value();

	VkSemaphore		compute_finish = context->CreateVkSemaphore().value();

	while (!window->ShouldClose())
	{

		vkResetCommandBuffer(cmd_buffer, 0);

		VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		vkBeginCommandBuffer(cmd_buffer, &begin);
		
		ptr<gvk::Image> back_buffer;
		VkSemaphore acquire_image_semaphore;
		uint32 image_index;

		if (auto v = context->AcquireNextImageAfterResize([&](uint32 w, uint32 h)
			{
				for (uint32 i = 0; i < context->GetBackBufferCount(); i++)
				{
					auto buffer = context->GetBackBuffers()[i];
					//general layout for storage image
					GvkDescriptorSetWrite().ImageWrite(descriptor_sets[i], "o_image", NULL, buffer->GetViews()[0],
						VK_IMAGE_LAYOUT_GENERAL).Emit(context->GetDevice());
				}
				return true;
			},&error))
		{
			auto [b, a, i] = v.value();
			back_buffer = b;
			acquire_image_semaphore = a;
			image_index = i;
		}
		else
		{
			printf("fail to resize swapchain reason %s", error.c_str());
			return 0;
		}

		GvkBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT).
			ImageBarrier(back_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT).Emit(cmd_buffer);

		float time = current_time();
		p_time.Update(cmd_buffer, &time);

		float speed = 1;
		p_speed.Update(cmd_buffer, &speed);

		uint32 width = context->GetBackBuffers()[0]->Info().extent.width, height = context->GetBackBuffers()[0]->Info().extent.height;
		vec2 resolution = { width,height};
		p_resolution.Update(cmd_buffer, &resolution);

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipeline());
		
		VkDescriptorSet binded_descriptor_sets[] = { descriptor_sets[image_index]->GetDescriptorSet()};
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
			pipeline->GetPipelineLayout(), 0, 1, binded_descriptor_sets, 0, NULL);

		vkCmdDispatch(cmd_buffer, (width + 15) / 16, (height + 15) / 16, 1);

		GvkBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
			.ImageBarrier(back_buffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_ACCESS_SHADER_WRITE_BIT, 0).Emit(cmd_buffer);

		vkEndCommandBuffer(cmd_buffer);

		queue->Submit(&cmd_buffer, 1, gvk::SemaphoreInfo().Signal(compute_finish).Wait(acquire_image_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT), NULL, true);

		context->Present(gvk::SemaphoreInfo().Wait(compute_finish, 0));

		window->UpdateWindow();
	}
}