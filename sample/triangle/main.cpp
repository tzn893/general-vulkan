#include "gvk.h"

#include "timer.h"
#include "image.h"

#include "gvk_shader_common.h"

struct TriangleVertex
{
	vec2 pos;
	vec2 uv;
};

#define require(expr,target) if(auto v = expr;v.has_value()) { target = v.value(); } else { gvk_assert(false);return -1; }

using namespace gvk;

int main() 
{
	VkFormat back_buffer_format;

	ptr<gvk::Window> window;
	require(gvk::Window::Create(600, 600, "triangle"), window);

	std::string error;
	ptr<gvk::Context> context;
	require(gvk::Context::CreateContext("triangle", GVK_VERSION{ 1,0,0 }, VK_API_VERSION_1_3 , window, &error),
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

	uint32 back_buffer_count = context->GetBackBufferCount();

	const char* include_directorys[] = { TRIANGLE_SHADER_DIRECTORY};

	auto vert = context->CompileShader("triangle.vert", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);
	auto frag = context->CompileShader("triangle.frag", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);

	GvkRenderPassCreateInfo render_pass_create;
	uint32 color_attachment = render_pass_create.AddAttachment(0, back_buffer_format,VK_SAMPLE_COUNT_1_BIT,
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
	
	GvkGraphicsPipelineCreateInfo graphic_pipeline_create(vert.value(), frag.value(), render_pass, 0);
	graphic_pipeline_create.rasterization_state.cullMode = VK_CULL_MODE_NONE;

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
		 {{0.5f, -0.5f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f}, {0.0f, 1.0f}},
		{{-0.5f, -0.5f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f}, {1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 1.0f}}
	};
	ptr<gvk::Buffer> buffer;
	require(context->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(vertexs),
		GVK_HOST_WRITE_RANDOM), buffer);
	buffer->Write(vertexs, 0, sizeof(vertexs));

	ptr<gvk::CommandQueue> queue;
	ptr<gvk::CommandPool>  pool;
	std::vector<VkCommandBuffer>	cmd_buffers(back_buffer_count);
	require(context->CreateQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT), queue);
	require(context->CreateCommandPool(queue.get()), pool);
	for(uint32 i = 0;i < back_buffer_count;i++)
		require(pool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY), cmd_buffers[i]);

	std::vector<VkSemaphore> color_output_finish(back_buffer_count);
	for(uint32 i = 0;i < back_buffer_count;i++)
	{
		require(context->CreateVkSemaphore(), color_output_finish[i]);
	}

	GvkPushConstant push_constant_time, push_constant_rotation;
	require(graphic_pipeline->GetPushConstantRange("time"), push_constant_time);
	require(graphic_pipeline->GetPushConstantRange("rotation"), push_constant_rotation);

	//load images
	ptr<gvk::Image> image;
	{
		auto [data, width, height] = load_image(ASSET_DIRECTORY + std::string("texture.jpg"));

		//create a staging buffer for data transfer
		ptr<gvk::Buffer> stageing_buffer;
		require(context->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			width * height * 4, GVK_HOST_WRITE_SEQUENTIAL), stageing_buffer);

		//write data from CPU to staging buffer
		stageing_buffer->Write(data, 0, width * height * 4);
		
		//create a image
		require(context->CreateImage(
			GvkImageCreateInfo::Image2D(VK_FORMAT_R8G8B8A8_UNORM, width, height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
		), image);
		
		//copy data from staging buffer to image
		queue->SubmitTemporalCommand(
			[&](VkCommandBuffer cmd_buffer) {
				GvkBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT)
					.ImageBarrier(image, 
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					0,
					VK_ACCESS_TRANSFER_WRITE_BIT).Emit(cmd_buffer);

				VkBufferImageCopy copy_info{};
				copy_info.bufferOffset = 0;
				copy_info.bufferRowLength = 0;
				copy_info.bufferImageHeight = 0;
				copy_info.imageExtent = image->Info().extent;
				copy_info.imageOffset = { 0 };
				copy_info.imageSubresource.aspectMask = gvk::GetAllAspects(image->Info().format);
				copy_info.imageSubresource.baseArrayLayer = 0;
				copy_info.imageSubresource.layerCount = image->Info().arrayLayers;
				copy_info.imageSubresource.mipLevel = 0;

				vkCmdCopyBufferToImage(cmd_buffer,stageing_buffer->GetBuffer(),
					image->GetImage(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy_info);

				GvkBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
					.ImageBarrier(image,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_ACCESS_TRANSFER_WRITE_BIT,
						VK_ACCESS_SHADER_READ_BIT).Emit(cmd_buffer);
			},gvk::SemaphoreInfo(),NULL,true
		);
	}

	VkImageView view;
	require(image->CreateView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D),view);

	ptr <gvk::DescriptorSetLayout> descriptor_set_layout;
	require(graphic_pipeline->GetInternalLayout(0, VK_SHADER_STAGE_FRAGMENT_BIT), descriptor_set_layout);
	ptr<gvk::DescriptorAllocator> descriptor_allocator = context->CreateDescriptorAllocator();
	ptr<gvk::DescriptorSet>	descriptor_set;
	require(descriptor_allocator->Allocate(descriptor_set_layout), descriptor_set);

	VkSampler sampler; 
	require(context->CreateSampler(GvkSamplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR,VK_SAMPLER_MIPMAP_MODE_LINEAR)), sampler);

	GvkDescriptorSetWrite()
		.ImageWrite(descriptor_set, "my_texture",sampler , view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		.Emit(context->GetDevice());

	uint32 i = 0;
	std::vector<VkFence> fence(back_buffer_count);
	for (uint32 i = 0;i < back_buffer_count;i++) 
	{
		//it is handy when implementing frame in flight
		require(context->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT), fence[i]);
	}
	while (!window->ShouldClose()) 
	{
		uint32 current_frame_idx = context->CurrentFrameIndex();
		vkWaitForFences(context->GetDevice(), 1, &fence[current_frame_idx], VK_TRUE, 0xffffffff);
		vkResetFences(context->GetDevice(), 1, &fence[current_frame_idx]);
		

		VkCommandBuffer cmd_buffer = cmd_buffers[current_frame_idx];
		vkResetCommandBuffer(cmd_buffer, 0);

		VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		begin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		ptr<gvk::Image> back_buffer;
		VkSemaphore acquire_image_semaphore;
		uint32	image_index = 0;
		
		std::string error;
		if (auto v = context->AcquireNextImageAfterResize(
			[&](uint32,uint32)
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
		,&error))
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
			VkDescriptorSet descriptor_sets[] = { descriptor_set->GetDescriptorSet() };
			vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphic_pipeline->GetPipelineLayout(),
				0, gvk_count_of(descriptor_sets), descriptor_sets, 0, NULL);

			float time = current_time();
			push_constant_time.Update(cmd_buffer, &time);

			mat4 rotation_mat{};
			rotation_mat.a00 =  sin(time); rotation_mat.a10 = cos(time); rotation_mat.a20 = 0;
			rotation_mat.a01 = -cos(time); rotation_mat.a11 = sin(time); rotation_mat.a21 = 0;
			rotation_mat.a02 =  0; rotation_mat.a12 =  0; rotation_mat.a22 = 1;
			rotation_mat.a33 = 1;

			push_constant_rotation.Update(cmd_buffer, &rotation_mat);

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
			.Signal(color_output_finish[current_frame_idx]), fence[current_frame_idx]
		);

		context->Present(gvk::SemaphoreInfo().Wait(color_output_finish[current_frame_idx], 0));

		window->UpdateWindow();
	}
	context->WaitForDeviceIdle();

	for (auto fb : frame_buffers) context->DestroyFrameBuffer(fb);
	for(auto sm : color_output_finish)  context->DestroyVkSemaphore(sm);
	for (auto f : fence) context->DestroyFence(f);
	context->DestroySampler(sampler);

	buffer = nullptr;
	graphic_pipeline = nullptr;
	window = nullptr;
}