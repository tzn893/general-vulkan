#include "gvk.h"

#include "timer.h"
#include "image.h"

#include "gvk_shader_common.h"

struct TriangleVertex
{
	vec3 color;
	vec3 pos;
};

struct ObjDesc
{
	uint64_t vertexAddr;
	uint64_t indiceAddr;
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
	device_create.AddDeviceExtension(GVK_DEVICE_EXTENSION_RAYTRACING);
	device_create.AddDeviceExtension(GVK_DEVICE_EXTENSION_BUFFER_DEVICE_ADDRESS);
	device_create.AddDeviceExtension(GVK_DEVICE_EXTENSION_INT64);
	device_create.RequireQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT, 1);

	context->InitializeDevice(device_create, &error);

	back_buffer_format = context->PickBackbufferFormatByHint({ VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_R8G8B8A8_UNORM });
	context->CreateSwapChain(back_buffer_format, &error);

	uint32 back_buffer_count = context->GetBackBufferCount();

	const char* include_directorys[] = { RT_SHADER_DIRECTORY};

	auto rchit = context->CompileShader("baseRT.rchit", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);
	auto rgen = context->CompileShader("baseRT.rgen", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);
	auto rmiss = context->CompileShader("baseRT.rmiss", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error);

	ptr<gvk::RaytracingPipeline> rt_pipeline;
	RayTracingPieplineCreateInfo rtPipelineCI;
	rtPipelineCI.AddRayGenerationShader(rgen.value());
	rtPipelineCI.AddRayIntersectionShader(nullptr, nullptr, rchit.value());
	rtPipelineCI.AddRayMissShader(rmiss.value());
	rtPipelineCI.SetMaxRecursiveDepth(1);

	rt_pipeline = context->CreateRaytracingPipeline(rtPipelineCI).value();
	auto back_buffers = context->GetBackBuffers();

	TriangleVertex vertices[] =
	{
		{{1.0f, 0.0f, 0.0f}, { 100, 100, 100}},
		{{0.0f, 1.0f, 0.0f}, { 100,-100, 100}},
		{{0.0f, 0.0f, 1.0f}, {-100, 100, 100}},
		{{1.0f, 1.0f, 1.0f}, {-100,-100, 100}}
	};

	uint32_t indices[] =
	{
		0, 1, 2,
		1, 2, 3
	};


	
	ptr<gvk::Buffer> vertBuffer, indBuffer;
	require(context->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		sizeof(vertices), GVK_HOST_WRITE_SEQUENTIAL), vertBuffer);
	vertBuffer->Write(vertices, 0, sizeof(vertices));

	require(context->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		sizeof(indices), GVK_HOST_WRITE_SEQUENTIAL), indBuffer);
	indBuffer->Write(indices, 0, sizeof(indices));


	ObjDesc desc;
	desc.vertexAddr = vertBuffer->GetAddress();
	desc.indiceAddr = indBuffer->GetAddress();
	
	ptr<gvk::Buffer> objDescBuffer;
	objDescBuffer = context->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(ObjDesc), GVK_HOST_WRITE_SEQUENTIAL).value();
	objDescBuffer->Write(&desc, 0, sizeof(ObjDesc));


	ptr<gvk::BottomAccelerationStructure> blas;
	ptr<gvk::TopAccelerationStructure> tlas;

	GvkBottomAccelerationStructureGeometryTriangles triangles;
	triangles.indiceBuffer = indBuffer;
	triangles.indiceType = VK_INDEX_TYPE_UINT32;

	triangles.vertexBuffer = vertBuffer;
	triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	triangles.vertexPositionAttributeOffset = sizeof(vec3);
	triangles.vertexStride = sizeof(TriangleVertex);

	triangles.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	triangles.firstVertexIndexOffset = 0;
	triangles.indexCount = 6;
	triangles.indexOffset = 0;
	triangles.transformOffset = 0;

	blas = context->CreateBottomAccelerationStructure(View<GvkBottomAccelerationStructureGeometryTriangles>(&triangles,0,1)).value();

	GvkTopAccelerationStructureInstance instance;
	instance.blas = blas;
	instance.instanceCustomIndex = 0;
	instance.mask = 0xff;
	instance.trans = VkTransformMatrix::Identity;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

	tlas = context->CreateTopAccelerationStructure(View<GvkTopAccelerationStructureInstance>(&instance, 0, 1)).value();
	
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

	ptr <gvk::DescriptorSetLayout> descriptor_set_layout;
	require(rt_pipeline->GetInternalLayout(0), descriptor_set_layout);
	ptr<gvk::DescriptorAllocator> descriptor_allocator = context->CreateDescriptorAllocator();
	ptr<gvk::DescriptorSet>	descriptor_set[3];
	for (uint32_t i = 0;i < 3;i++)
	{
		require(descriptor_allocator->Allocate(descriptor_set_layout), descriptor_set[i]);

		GvkDescriptorSetWrite()
			.AccelerationStructureWrite(descriptor_set[i], 0, tlas->GetAS())
			.ImageWrite(descriptor_set[i], "o_image", NULL, back_buffers[i]->GetViews()[0], VK_IMAGE_LAYOUT_GENERAL)
			.BufferWrite(descriptor_set[i], "objDesc", objDescBuffer->GetBuffer(), 0, sizeof(ObjDesc))
			.Emit(context->GetDevice());
	}

	//TODO set back buffer

	uint32 i = 0;
	/*
	std::vector<VkFence> fence(back_buffer_count);
	for (uint32 i = 0;i < back_buffer_count;i++) 
	{
		//it is handy when implementing frame in flight
		require(context->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT), fence[i]);

	}
	*/
	VkFence fence = context->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT).value();
	while (!window->ShouldClose()) 
	{
		uint32 current_frame_idx = context->CurrentFrameIndex();
		vkWaitForFences(context->GetDevice(), 1, &fence, VK_TRUE, 0xffffffff);
		vkResetFences(context->GetDevice(), 1, &fence);
		

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
		
		GvkBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR).
			ImageBarrier(back_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				0, VK_ACCESS_SHADER_WRITE_BIT).Emit(cmd_buffer);
		
		GvkDescriptorSetBindingUpdate(cmd_buffer, rt_pipeline)
		.BindDescriptorSet(descriptor_set[image_index])
		.Update();

		rt_pipeline->TraceRay(cmd_buffer, 600, 600, 1);
		
		GvkBarrier(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT).
			ImageBarrier(back_buffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_ACCESS_SHADER_WRITE_BIT, 0).Emit(cmd_buffer);

		
		vkEndCommandBuffer(cmd_buffer);

		queue->Submit(&cmd_buffer, 1,
			gvk::SemaphoreInfo()
			.Wait(acquire_image_semaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
			.Signal(color_output_finish[current_frame_idx]), fence
		);

		context->Present(gvk::SemaphoreInfo().Wait(color_output_finish[current_frame_idx], 0));
		window->UpdateWindow();
	}
	context->WaitForDeviceIdle();

	for(auto sm : color_output_finish)  context->DestroyVkSemaphore(sm);
	context->DestroyFence(fence);

	vertBuffer = nullptr;
	indBuffer = nullptr;
	rt_pipeline = nullptr;
	window = nullptr;
}