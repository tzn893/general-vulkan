#pragma once
#include "gvk_command.h"
#include "gvk_resource.h"
#include "gvk_shader.h"
#include "gvk_pipeline.h"


namespace gvk
{
	class Context;

	namespace VkTransformMatrix
	{
		inline constexpr VkTransformMatrixKHR Identity = { {{1,0,0,0},{0,1,0,0},{0,0,1,0}} };
	}

	struct RayTracingPieplineCreateInfo
	{
		struct ShaderGroup
		{
			ptr<Shader> rayGeneration;
			ptr<Shader> rayMiss;
			struct 
			{
				ptr<Shader> closestHit;
				ptr<Shader> anyHit;
				ptr<Shader> intersection;
			} rayIntersection;
			VkShaderStageFlags stages;
		};

		std::vector<ShaderGroup> rayGenShaderGroup;
		std::vector<ShaderGroup> rayMissShaderGroup;
		std::vector<ShaderGroup> rayHitShaderGroup;


		uint32_t				 maxRecursiveDepth = 5;

		std::vector<ShaderGroup> GetShaderGroups() const;
		uint32_t AddRayGenerationShader(ptr<Shader> rayGeneration);
		uint32_t AddRayMissShader(ptr<Shader> rayMiss);
		uint32_t AddRayIntersectionShader(ptr<Shader> intersection, ptr<Shader> anyHit, ptr<Shader> closestHit);
		void SetMaxRecursiveDepth(uint32_t depth);
	};


	struct GvkBottomAccelerationStructureGeometryTriangles
	{
		VkFormat vertexFormat;
		ptr<Buffer>	vertexBuffer;
		size_t vertexStride;
		uint32_t vertexPositionAttributeOffset;
		uint32_t maxVertexCount;

		ptr<Buffer> indiceBuffer;
		VkIndexType indiceType;
		VkGeometryFlagsKHR flags;

		VkAccelerationStructureBuildRangeInfoKHR range;
	};

	class BottomAccelerationStructure
	{
	public:
		BottomAccelerationStructure(Context* ctx, VkAccelerationStructureKHR accel,const GvkBottomAccelerationStructureGeometryTriangles* info, uint32_t size, ptr<Buffer> as);
		~BottomAccelerationStructure();

		const GvkBottomAccelerationStructureGeometryTriangles& Info(uint32_t idx) const { return m_Info[idx]; }

		VkAccelerationStructureKHR GetAS() { return m_accelStruct; }
		VkDeviceAddress			   GetASDeviceAddress();

	private:
		VkAccelerationStructureKHR m_accelStruct;
		std::vector<GvkBottomAccelerationStructureGeometryTriangles> m_Info;
		ptr<Buffer> m_accelStructBuffer;

		Context* m_Ctx;
	};

	struct GvkTopAccelerationStructureInstance
	{
		VkTransformMatrixKHR trans;
		ptr<BottomAccelerationStructure> blas;
		uint32_t             instanceCustomIndex;
		uint8_t				 mask;
		VkGeometryInstanceFlagsKHR flags;
	};

	class TopAccelerationStructure
	{
	public:
		TopAccelerationStructure(Context* ctx, VkAccelerationStructureKHR accel, const GvkTopAccelerationStructureInstance* info, uint32_t count, ptr<Buffer> as);
		~TopAccelerationStructure();

		const GvkTopAccelerationStructureInstance& Info(uint32_t idx) const { return m_Info[idx]; }
		VkAccelerationStructureKHR GetAS() { return m_accelStruct; }


	private:
		VkAccelerationStructureKHR m_accelStruct;
		std::vector<GvkTopAccelerationStructureInstance> m_Info;
		ptr<Buffer> m_accelStructBuffer;
		Context* m_Ctx;
	};

	class RaytracingPipeline : public Pipeline
	{
	public:
		virtual ~RaytracingPipeline() override;

		RayTracingPieplineCreateInfo&	Info();

		void							TraceRay(VkCommandBuffer cmd, uint32_t dispatchX, uint32_t dispatchY, uint32_t dispatchZ);

	private:
		RaytracingPipeline(VkPipeline pipeline, VkPipelineLayout layout, const std::vector<ptr<DescriptorSetLayout>>& descriptor_set_layouts, const std::unordered_map<std::string, VkPushConstantRange>& push_constants,
			ptr<RenderPass> render_pass, uint32_t subpass_index, VkPipelineBindPoint bind_point, VkDevice device,const RayTracingPieplineCreateInfo& createInfo);

		RayTracingPieplineCreateInfo  m_Info;
		ptr<gvk::Buffer> m_SBT;

		VkStridedDeviceAddressRegionKHR rayGen;
		VkStridedDeviceAddressRegionKHR miss;
		VkStridedDeviceAddressRegionKHR hit;
		VkStridedDeviceAddressRegionKHR callable;

		friend class Context;
	};
}
