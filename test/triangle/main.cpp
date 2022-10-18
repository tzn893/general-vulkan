#include "gvk.h"
#include "shader.h"

#define require(expr,target) if(auto v = expr;v.has_value()) { target = v.value(); } else { gvk_assert(false);return -1; }
constexpr VkFormat back_buffer_foramt = VK_FORMAT_B8G8R8A8_UNORM;

int main() 
{
	ptr<gvk::Window> window;
	require(gvk::Window::Create(1000, 800, "triangle"), window);

	std::string error;
	ptr<gvk::Context> context;
	require(gvk::Context::CreateContext("triangle", GVK_VERSION{ 1,0,0 }, VK_API_VERSION_1_3 , window, &error),
		context);

	GvkInstanceCreateInfo instance_create;
	instance_create.AddInstanceExtension(GVK_INSTANCE_EXTENSION_DEBUG);
	instance_create.AddLayer(GVK_LAYER_DEBUG);
	
	context->InitializeInstance(instance_create, &error);

	GvkDeviceCreateInfo device_create;
	device_create.AddDeviceExtension(GVK_DEVICE_EXTENSION_SWAP_CHAIN);
	device_create.RequireQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT, 1);

	context->InitializeDevice(device_create, &error);

	context->CreateSwapChain(window.get(), back_buffer_foramt, &error);

	const char* include_directorys[] = { TRIANGLE_SHADER_DIRECTORY, SHADER_DIRECTORY };

	auto vert = context->CompileShader("triangle.vert", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);
	auto frag = context->CompileShader("triangle.frag", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);

	GvkRenderPassCreateInfo render_pass_create;
	uint32 color_attachment = render_pass_create.AddAttachment(0, back_buffer_foramt,VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	render_pass_create.AddSubpass(0, VK_PIPELINE_BIND_POINT_GRAPHICS);
	render_pass_create.AddSubpassColorAttachment(0, color_attachment);

	render_pass_create.AddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	
	ptr<gvk::RenderPass> render_pass;
	if (auto v = context->CreateRenderPass(render_pass_create); v.has_value())
	{
		render_pass = v.value();
	}
	else 
	{
		return 0;
	}
	
	GvkGraphicsPipelineCreateInfo graphic_pipeline_create;
	graphic_pipeline_create.vertex_shader = vert.value();
	graphic_pipeline_create.fragment_shader = frag.value();

	graphic_pipeline_create.rasterization_state.cullMode = VK_CULL_MODE_NONE;
	graphic_pipeline_create.frame_buffer_blend_state.Resize(1);
	graphic_pipeline_create.frame_buffer_blend_state.Set(0, GvkGraphicsPipelineCreateInfo::BlendState());
	
	graphic_pipeline_create.target_pass = render_pass;
	graphic_pipeline_create.subpass_index = 0;

	ptr<gvk::Pipeline> graphic_pipeline;
	require(context->CreateGraphicsPipeline(graphic_pipeline_create), graphic_pipeline);

	auto back_buffers = context->GetBackBuffers();

	std::vector<VkFramebuffer> frame_buffers(back_buffers.size());
	for (uint32 i = 0;i < back_buffers.size();i++)
	{
		ptr<gvk::Image> back_buffer = back_buffers[i];
		require(context->CreateFrameBuffer(render_pass, &back_buffer->GetViews()[0],
			back_buffer->Info().extent.width, back_buffer->Info().extent.height), frame_buffers[i]);
	}

	TriangleVertex vertexs[] = {
		 {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};
	ptr<gvk::Buffer> buffer;
	require(context->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(vertexs),
		GVK_HOST_WRITE_SEQUENTIAL), buffer);
	buffer->Write(vertexs, 0, sizeof(vertexs));

	ptr<gvk::CommandQueue> queue;
	ptr<gvk::CommandPool>  pool;
	VkCommandBuffer		   cmd_buffer;
	require(context->CreateQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT), queue);
	require(context->CreateCommandPool(queue.get()), pool);
	require(pool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY), cmd_buffer);

	VkSemaphore color_output_finish;
	require(context->CreateVkSemaphore(),color_output_finish);


	while (!window->ShouldClose()) 
	{
		window->UpdateWindow();

		vkResetCommandBuffer(cmd_buffer, 0);

		VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		begin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		ptr<gvk::Image> back_buffer;
		VkSemaphore acquire_image_semaphore;
		if (auto v = context->AcquireNextImage(); v.has_value()) 
		{
			auto [b,a] = v.value();
			back_buffer = b;
			acquire_image_semaphore = a;
		}
		else
		{
			return 0;
		}

		if (vkBeginCommandBuffer(cmd_buffer, &begin) != VK_SUCCESS) 
		{
			return 0;
		}

		//body of recording commands
		
		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphic_pipeline->GetPipeline());

		VkClearValue cv;
		cv.color = VkClearColorValue{ {0.1f,0.1f,0.5f,1.0f} };

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)back_buffer->Info().extent.width;
		viewport.height = (float)back_buffer->Info().extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = VkExtent2D{ back_buffer->Info().extent.width,
			back_buffer->Info().extent.height };

		render_pass->Begin(frame_buffers[context->CurrentFrameIndex()],
			&cv,
			{ {},VkExtent2D{ back_buffer->Info().extent.width,back_buffer->Info().extent.height} }, 
			viewport,
			scissor,
			cmd_buffer
		).Record([&]() {
			vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphic_pipeline->GetPipeline());

			VkBuffer vbuffers[] = { buffer->GetBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vbuffers, offsets);

			vkCmdDraw(cmd_buffer, gvk_count_of(vertexs), 1, 0, 0);
			}
		);

		vkEndCommandBuffer(cmd_buffer);

		queue->Submit(&cmd_buffer, 1,
			gvk::SemaphoreInfo()
			.Wait(acquire_image_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
			.Signal(color_output_finish)
		);

		context->Present(gvk::SemaphoreInfo().Wait(color_output_finish,0));

		queue->StallForDevice();
	}
}