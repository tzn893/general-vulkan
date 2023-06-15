#include "gvk.h"
using namespace gvk;

int main()
{
#define require(expr,target) if(auto v = expr;v.has_value()) { target = v.value(); } else { gvk_assert(false);return -1; }

	ptr<gvk::Window> window;
	require(gvk::Window::Create(600, 600, "compute"), window);

	std::string error;
	ptr<gvk::Context> context;
	require(gvk::Context::CreateContext("compute", GVK_VERSION{ 1,0,0 }, VK_API_VERSION_1_3, window, &error),
		context);

	GvkInstanceCreateInfo instance_create;
	instance_create.AddInstanceExtension(GVK_INSTANCE_EXTENSION_DEBUG);
	instance_create.AddInstanceExtension(GVK_INSTANCE_EXTENSION_SHADER_PRINT);
	instance_create.AddLayer(GVK_LAYER_DEBUG);
	instance_create.AddLayer(GVK_LAYER_FPS_MONITOR);
	instance_create.custom_debug_callback = debug_callback_filtered;

	context->InitializeInstance(instance_create, &error);

	GvkDeviceCreateInfo device_create;
	device_create.AddDeviceExtension(GVK_DEVICE_EXTENSION_SWAP_CHAIN);
	device_create.RequireQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT, 1);

	context->InitializeDevice(device_create, &error);

	auto queue = context->CreateQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT).value();

	const char* include_directorys[] = { DEBUG_PRINT_SHADER_DIRECTORY };

	auto comp = context->CompileShader("debug_print.comp", gvk::ShaderMacros(),
		include_directorys, gvk_count_of(include_directorys),
		include_directorys, gvk_count_of(include_directorys),
		&error).value();

	GvkComputePipelineCreateInfo pipeline_CI{};
	pipeline_CI.shader = comp;

	ptr<gvk::Pipeline> compute_pipeline;
	require(context->CreateComputePipeline(pipeline_CI), compute_pipeline);

	queue->SubmitTemporalCommand([&](VkCommandBuffer cmd)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline->GetPipeline());
			vkCmdDispatch(cmd, 100, 100, 10);
		},gvk::SemaphoreInfo(),NULL,true
	);

}