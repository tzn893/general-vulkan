#include "gvk_raytracing.h"
#include "gvk_context.h"

namespace gvk
{

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingProperties(gvk::Context* ctx)
	{
		static VkPhysicalDeviceRayTracingPipelinePropertiesKHR props{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		static bool initialized = false;

		if (!initialized)
		{
			VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
			prop2.pNext = &props;

			VkPhysicalDevice device = ctx->GetPhysicalDevice();
			vkGetPhysicalDeviceProperties2(device, &prop2);
		}

		return props;
	}


	std::vector<RayTracingPieplineCreateInfo::ShaderGroup> RayTracingPieplineCreateInfo::GetShaderGroups() const
	{
		// order ray gen/ray miss/hit
		std::vector<RayTracingPieplineCreateInfo::ShaderGroup> rvShaderGroup = rayGenShaderGroup;
		rvShaderGroup.insert(rvShaderGroup.end(), rayMissShaderGroup.begin(), rayMissShaderGroup.end());
		rvShaderGroup.insert(rvShaderGroup.end(), rayHitShaderGroup.begin(), rayHitShaderGroup.end());

		return rvShaderGroup;
	}

	uint32_t gvk::RayTracingPieplineCreateInfo::AddRayGenerationShader(ptr<Shader> rayGeneration)
	{
		ShaderGroup group;
		group.rayGeneration = rayGeneration;
		group.stages = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		uint32_t groupIdx = rayGenShaderGroup.size();

		rayGenShaderGroup.push_back(group);
		return groupIdx;
	}

	uint32_t gvk::RayTracingPieplineCreateInfo::AddRayMissShader(ptr<Shader> rayMiss)
	{
		ShaderGroup group;
		group.rayMiss = rayMiss;
		group.stages = VK_SHADER_STAGE_MISS_BIT_KHR;
		uint32_t groupIdx = rayMissShaderGroup.size();

		rayMissShaderGroup.push_back(group);
		return groupIdx;
	}

	uint32_t gvk::RayTracingPieplineCreateInfo::AddRayIntersectionShader(ptr<Shader> intersection, ptr<Shader> anyHit, ptr<Shader> closestHit)
	{
		VkShaderStageFlags shaderStages{};
		ShaderGroup group;
		

		gvk_assert(intersection != nullptr || anyHit != nullptr || closestHit != nullptr)
		if (intersection != nullptr)
		{
			group.rayIntersection.intersection = intersection;
			shaderStages |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		}

		if (anyHit != nullptr)
		{
			group.rayIntersection.anyHit = anyHit;
			shaderStages |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		}

		if (closestHit != nullptr)
		{
			group.rayIntersection.closestHit = closestHit;
			shaderStages |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		}
		group.stages = shaderStages;
		uint32_t groupIdx = rayHitShaderGroup.size();

		rayHitShaderGroup.push_back(group);
		return groupIdx;
	}


	RaytracingPipeline::RaytracingPipeline(VkPipeline pipeline, VkPipelineLayout layout, const std::vector<ptr<DescriptorSetLayout>>& descriptor_set_layouts, const std::unordered_map<std::string, VkPushConstantRange>& push_constants,
		ptr<RenderPass> render_pass, uint32_t subpass_index, VkPipelineBindPoint bind_point, VkDevice device,const RayTracingPieplineCreateInfo& createInfo)
		:Pipeline(pipeline, layout, descriptor_set_layouts, push_constants, render_pass, subpass_index, bind_point, device)
	{
		m_Info = createInfo;
	}

	void RayTracingPieplineCreateInfo::SetMaxRecursiveDepth(uint32_t depth)
	{
		maxRecursiveDepth = depth;
	}
	RaytracingPipeline::~RaytracingPipeline()
	{
		m_SBT = nullptr;
	}

	RayTracingPieplineCreateInfo& RaytracingPipeline::Info()
	{
		// TODO: 在此处插入 return 语句
		return m_Info;
	}

	void RaytracingPipeline::TraceRay(VkCommandBuffer cmd, uint32_t dispatchX, uint32_t dispatchY, uint32_t dispatchZ)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_Pipeline);

		vkCmdTraceRaysKHR(cmd, &rayGen, &miss, &hit, &callable, dispatchX, dispatchY, dispatchZ);
	}
	
	TopAccelerationStructure::TopAccelerationStructure(Context* ctx, VkAccelerationStructureKHR accel, const GvkTopAccelerationStructureInstance* info, uint32_t count, ptr<Buffer> as)
		:m_accelStruct(accel), m_Ctx(ctx), m_accelStructBuffer(as)
	{
		for (uint32_t i = 0;i < count;i++)
		{
			m_Info.push_back(info[i]);
		}
	}

	TopAccelerationStructure::~TopAccelerationStructure()
	{
		VkDevice device = m_Ctx->GetDevice();
		vkDestroyAccelerationStructureKHR(device, m_accelStruct, NULL);
		m_accelStructBuffer = nullptr;
	}

	BottomAccelerationStructure::BottomAccelerationStructure(Context* ctx, VkAccelerationStructureKHR accel, const GvkBottomAccelerationStructureGeometryTriangles* info, uint32_t count, ptr<Buffer> as)
		:m_accelStruct(accel), m_Ctx(ctx), m_accelStructBuffer(as)
	{
		for (uint32_t i = 0;i < count;i++)
		{
			m_Info.push_back(info[i]);
		}
	}

	BottomAccelerationStructure::~BottomAccelerationStructure()
	{
		VkDevice device = m_Ctx->GetDevice();
		vkDestroyAccelerationStructureKHR(device, m_accelStruct, NULL);
		m_accelStructBuffer = nullptr;
	}

	VkDeviceAddress BottomAccelerationStructure::GetASDeviceAddress()
	{
		VkAccelerationStructureDeviceAddressInfoKHR deviceAddress{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
		deviceAddress.accelerationStructure = m_accelStruct;

		return vkGetAccelerationStructureDeviceAddressKHR(m_Ctx->GetDevice(), &deviceAddress);
	}

	opt<ptr<TopAccelerationStructure>> Context::CreateTopAccelerationStructure(View<GvkTopAccelerationStructureInstance> topAsInfo)
	{
		std::vector<VkAccelerationStructureInstanceKHR> instances(topAsInfo.size());
		// collect geometry data
		for (uint32_t i = 0; i < topAsInfo.size(); i++)
		{
			const GvkTopAccelerationStructureInstance& instance = topAsInfo[i];

			VkAccelerationStructureInstanceKHR vkInstance{};
			vkInstance.transform = instance.trans;
			vkInstance.mask = instance.mask;
			vkInstance.instanceCustomIndex = instance.instanceCustomIndex;
			vkInstance.flags = instance.flags;
			/*
				accelerationStructureReference is either:
				a device address containing the value obtained from vkGetAccelerationStructureDeviceAddressKHR or vkGetAccelerationStructureHandleNV (used by device operations which reference acceleration structures) or,
				a VkAccelerationStructureKHR object (used by host operations which reference acceleration structures).
			*/
			vkInstance.accelerationStructureReference = (uint64_t)instance.blas->GetASDeviceAddress();
			instances[i] = vkInstance;
		}

		// upload instance data to a gpu buffer
		uint32_t instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
		ptr<Buffer> instanceBuffer = CreateBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			instanceBufferSize, GVK_HOST_WRITE_SEQUENTIAL).value();
		instanceBuffer->Write(instances.data(), 0, instanceBufferSize);

		VkAccelerationStructureGeometryKHR geom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		geom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		geom.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		geom.geometry.instances.data.deviceAddress = instanceBuffer->GetAddress();

		VkAccelerationStructureBuildGeometryInfoKHR asGI{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		asGI.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		asGI.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		asGI.geometryCount = 1;
		asGI.pGeometries = &geom;

		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		uint32_t maxPrimitiveCounts = topAsInfo.size();

		vkGetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&asGI, &maxPrimitiveCounts, &sizeInfo);

		ptr<Buffer> accelStructBuffer = CreateBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, sizeInfo.accelerationStructureSize,
			GVK_HOST_WRITE_NONE).value();

		VkAccelerationStructureCreateInfoKHR accelCI{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		accelCI.size = sizeInfo.accelerationStructureSize;
		accelCI.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelCI.buffer = accelStructBuffer->GetBuffer();

		VkAccelerationStructureKHR accelS;
		vkCreateAccelerationStructureKHR(m_Device, &accelCI, NULL, &accelS);

		ptr<Buffer> scratchBuffer = CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			sizeInfo.buildScratchSize, GVK_HOST_WRITE_NONE).value();

		asGI.srcAccelerationStructure = NULL;
		asGI.dstAccelerationStructure = accelS;
		asGI.scratchData.deviceAddress = scratchBuffer->GetAddress();
		VkAccelerationStructureBuildRangeInfoKHR        buildOffsetInfo{ instances.size(), 0, 0, 0 };


		m_PresentQueue->SubmitTemporalCommand(
			[&](VkCommandBuffer cmd)
			{
				VkAccelerationStructureBuildRangeInfoKHR* ranges[] = { &buildOffsetInfo };
		vkCmdBuildAccelerationStructuresKHR(cmd, 1, &asGI, ranges);
			}, gvk::SemaphoreInfo::None(), NULL, true
				);
		return std::make_shared<TopAccelerationStructure>(this, accelS, topAsInfo.GetData(), topAsInfo.size(), accelStructBuffer);
	}

	opt<ptr<BottomAccelerationStructure>> Context::CreateBottomAccelerationStructure(View<GvkBottomAccelerationStructureGeometryTriangles> bottomASInfo)
	{
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> geomRanges(bottomASInfo.size());
		std::vector<VkAccelerationStructureGeometryKHR > geoms(bottomASInfo.size());
		// collect geometry data
		for (uint32_t i = 0; i < bottomASInfo.size(); i++)
		{
			const GvkBottomAccelerationStructureGeometryTriangles& tri = bottomASInfo[i];
			VkAccelerationStructureGeometryTrianglesDataKHR vkTriangleGeom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};

			if (tri.indiceType != VK_INDEX_TYPE_NONE_KHR)
			{
				vkTriangleGeom.indexData.deviceAddress = tri.indiceBuffer->GetAddress();
			}
			vkTriangleGeom.indexType = tri.indiceType;

			vkTriangleGeom.vertexFormat = tri.vertexFormat;
			vkTriangleGeom.vertexStride = tri.vertexStride;
			uint32_t vertexMemoryOffset = tri.vertexPositionAttributeOffset;
			vkTriangleGeom.vertexData.deviceAddress = tri.vertexBuffer->GetAddress() + vertexMemoryOffset;
			vkTriangleGeom.maxVertex = tri.maxVertexCount;


			VkAccelerationStructureGeometryKHR geom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
			geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			geom.geometry.triangles = vkTriangleGeom;
			geom.flags = tri.flags;

			geoms[i] = geom;
			geomRanges[i] = tri.range;
		}


		VkAccelerationStructureBuildGeometryInfoKHR asGI{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		asGI.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		asGI.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		asGI.geometryCount = geoms.size();
		asGI.pGeometries = geoms.data();

		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		std::vector<uint32_t> maxPrimitiveCounts(bottomASInfo.size());
		for (uint32_t i = 0;i < bottomASInfo.size();i++)
		{
			maxPrimitiveCounts[i] = bottomASInfo[i].range.primitiveCount;
		}

		vkGetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&asGI, maxPrimitiveCounts.data(), &sizeInfo);
		
		ptr<Buffer> accelStructBuffer = CreateBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, sizeInfo.accelerationStructureSize,
			GVK_HOST_WRITE_NONE).value();

		VkAccelerationStructureCreateInfoKHR accelCI{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		accelCI.size = sizeInfo.accelerationStructureSize;
		accelCI.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelCI.buffer = accelStructBuffer->GetBuffer();

		VkAccelerationStructureKHR accelS;
		vkCreateAccelerationStructureKHR(m_Device, &accelCI, NULL, &accelS);

		ptr<Buffer> scratchBuffer = CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			sizeInfo.buildScratchSize, GVK_HOST_WRITE_NONE).value();

		asGI.srcAccelerationStructure = NULL;
		asGI.dstAccelerationStructure = accelS;
		asGI.scratchData.deviceAddress = scratchBuffer->GetAddress();


		m_PresentQueue->SubmitTemporalCommand(
			[&](VkCommandBuffer cmd)
			{
				VkAccelerationStructureBuildRangeInfoKHR* ranges[] = { geomRanges.data()};
				vkCmdBuildAccelerationStructuresKHR(cmd, 1, &asGI, ranges);
			}, gvk::SemaphoreInfo::None(), NULL, true
				);
		return std::make_shared<BottomAccelerationStructure>(this, accelS, bottomASInfo.GetData(), bottomASInfo.size(), accelStructBuffer);
	}


}

