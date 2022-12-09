#include "gvk.h"
#include "shader.h"
#include <chrono>




#define require(expr,target) if(auto v = expr;v.has_value()) { target = v.value(); } else { gvk_assert(false);return -1; }

using namespace gvk;

float current_time() {

	using namespace std::chrono;
	static uint64_t start = 0;
	if (start == 0) start = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	return (float)(ms - start) / 1000.f;
}

int main()
{
	VkFormat back_buffer_format;

	ptr<gvk::Window> window;
	require(gvk::Window::Create(600, 600, "triangle"), window);

	std::string error;
	ptr<gvk::Context> context;
	require(gvk::Context::CreateContext("triangle", GVK_VERSION{ 1,0,0 }, VK_API_VERSION_1_3, window, &error),
		context);

	current_time();

	GvkInstanceCreateInfo instance_create;
	instance_create.AddInstanceExtension(GVK_INSTANCE_EXTENSION_DEBUG);
	instance_create.AddLayer(GVK_LAYER_DEBUG);
	instance_create.AddLayer(GVK_LAYER_FPS_MONITOR);

	context->InitializeInstance(instance_create, &error);

	GvkDeviceCreateInfo device_create;
	device_create.AddDeviceExtension(GVK_DEVICE_EXTENSION_SWAP_CHAIN);
	device_create.AddDeviceExtension(GVK_DEVICE_EXTENSION_GEOMETRY_SHADER);
	device_create.RequireQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT, 1);

	context->InitializeDevice(device_create, &error);

	back_buffer_format = context->PickBackbufferFormatByHint({ VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_R8G8B8A8_UNORM });
	context->CreateSwapChain(back_buffer_format, &error);

	const char* include_directorys[] = { GEOMETRY_SHADER_DIRECTORY };

	auto vert = context->CompileShader("geom.vert", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);
	auto frag = context->CompileShader("geom.frag", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);
	auto geom = context->CompileShader("geom.geom", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);

	GvkRenderPassCreateInfo render_pass_create;
	uint32 color_attachment = render_pass_create.AddAttachment(0, back_buffer_format, VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

	std::vector<GvkGraphicsPipelineCreateInfo::BlendState> blend_states(1, GvkGraphicsPipelineCreateInfo::BlendState());;
	GvkGraphicsPipelineCreateInfo graphic_pipeline_create(vert.value(), frag.value(), render_pass, 0, blend_states.data());
	graphic_pipeline_create.rasterization_state.cullMode = VK_CULL_MODE_NONE;
	graphic_pipeline_create.geometry_shader = geom.value();
	//Unfortunately glsl reflect can't access primitive topology so we have to set it manually
	graphic_pipeline_create.input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	graphic_pipeline_create.rasterization_state.cullMode = VK_CULL_MODE_NONE;

	ptr<gvk::Pipeline> graphic_pipeline;
	require(context->CreateGraphicsPipeline(graphic_pipeline_create), graphic_pipeline);

	auto back_buffers = context->GetBackBuffers();

	std::vector<VkFramebuffer> frame_buffers(back_buffers.size());
	for (uint32 i = 0; i < back_buffers.size(); i++)
	{
		ptr<gvk::Image> back_buffer = back_buffers[i];
		require(context->CreateFrameBuffer(render_pass, &back_buffer->GetViews()[0],
			back_buffer->Info().extent.width, back_buffer->Info().extent.height), frame_buffers[i]);
	}

	GPoint points[] = { {vec3{ 0.3, 0.3,0.},vec3{1.,1.,1.}},
					   {vec3{ 0.3,-0.3,0.},vec3{1.,1.,1.}},
					   {vec3{-0.3,-0.3,0.},vec3{1.,1.,1.}},
					   {vec3{-0.3, 0.3,0.},vec3{1.,1.,1.}}
	};
	
	ptr<gvk::Buffer> buffer;
	require(context->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(points),
		GVK_HOST_WRITE_RANDOM), buffer);
	buffer->Write(points, 0, sizeof(points));

	ptr<gvk::CommandQueue> queue;
	ptr<gvk::CommandPool>  pool;
	VkCommandBuffer		   cmd_buffer;
	require(context->CreateQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT), queue);
	require(context->CreateCommandPool(queue.get()), pool);
	require(pool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY), cmd_buffer);

	VkSemaphore color_output_finish;
	require(context->CreateVkSemaphore(), color_output_finish);

	uint32 i = 0;
	VkFence fence;
	require(context->CreateFence(0), fence);

	GvkPushConstant rotation;
	require(graphic_pipeline->GetPushConstantRange("rotation"), rotation);

	while (!window->ShouldClose())
	{
		vkResetFences(context->GetDevice(), 1, &fence);

		vkResetCommandBuffer(cmd_buffer, 0);

		VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		ptr<gvk::Image> back_buffer;
		VkSemaphore acquire_image_semaphore;
		uint32 image_index;
		
		std::string error;
		if (auto v = context->AcquireNextImageAfterResize(
			[&](uint32, uint32)
			{
				auto back_buffers = context->GetBackBuffers();
				//recreate all the framebuffers
				for (uint32 i = 0; i < back_buffers.size(); i++)
				{
					auto back_buffer = back_buffers[i];
					context->DestroyFrameBuffer(frame_buffers[i]);
					if (auto v = context->CreateFrameBuffer(render_pass, &back_buffer->GetViews()[0],
						back_buffer->Info().extent.width, back_buffer->Info().extent.height); v.has_value())
						frame_buffers[i] = v.value();
					else
						return false;
				}
				return true;
			}
			, &error))
		{
			auto [b, a, i] = v.value();
			back_buffer = b;
			acquire_image_semaphore = a;
			image_index = i;
		}
		else
		{
			printf("%s", error.c_str());
			break;
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

		render_pass->Begin(frame_buffers[image_index],
			&cv,
			{ {},VkExtent2D{ back_buffer->Info().extent.width,back_buffer->Info().extent.height} },
			viewport,
			scissor,
			cmd_buffer
		).Record([&]() {

			float time = current_time() * 10.f;
			mat3 rotate;
			rotate.a00 = cos(time), rotate.a01 = sin(time), rotate.a02 = 0.f;
			rotate.a10 = -sin(time), rotate.a11 = cos(time), rotate.a12 = 0.f;
			rotate.a20 = 0, rotate.a21 = 0, rotate.a22 = 1.f;
			rotation.Update(cmd_buffer, &rotate);

			VkBuffer vbuffers[] = { buffer->GetBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vbuffers, offsets);

			vkCmdDraw(cmd_buffer, gvk_count_of(points), 1, 0, 0);
			}
		);


		vkEndCommandBuffer(cmd_buffer);

		queue->Submit(&cmd_buffer, 1,
			gvk::SemaphoreInfo()
			.Wait(acquire_image_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
			.Signal(color_output_finish), fence
		);

		context->Present(gvk::SemaphoreInfo().Wait(color_output_finish, 0));

		window->UpdateWindow();

		vkWaitForFences(context->GetDevice(), 1, &fence, VK_TRUE, 0xffffffff);
	}

	for (auto fb : frame_buffers) context->DestroyFrameBuffer(fb);
	context->DestroyVkSemaphore(color_output_finish);

	buffer = nullptr;
	graphic_pipeline = nullptr;
	window = nullptr;
}