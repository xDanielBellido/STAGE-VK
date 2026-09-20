#include "GeometryPass.h"

#include "VulkanAllocator.h"
#include "VulkanUploadContext.h"

#include <array>

bool GeometryPass::Init(VkDevice device, const VulkanAllocator& allocator, VulkanUploadContext& uploadContext)
{
    CreateTriangleVertexBuffer(allocator, uploadContext);

    VulkanShaderModule vertexShader;
    VulkanShaderModule fragmentShader;

    if (!vertexShader.Init(device, "CompiledShaders/GeometryVS.spv"))
    {
        return false;
    }

    if (!fragmentShader.Init(device, "CompiledShaders/GeometryPS.spv"))
    {
        return false;
    }

    return true;
}

void GeometryPass::CleanUp()
{
    vertexBuffer.CleanUp();
    vertexCount = 0;
}

void GeometryPass::CreateTriangleVertexBuffer(const VulkanAllocator& allocator, VulkanUploadContext& uploadContext)
{
    static constexpr std::array<Vertex, 3> vertices =
    {
        Vertex{ -1.0f, -1.0f, 0.0f },
        Vertex{  0.0f,  1.0f, 0.0f },
        Vertex{  1.0f, -1.0f, 0.0f }
    };

    const VkDeviceSize bufferSize = sizeof(vertices);

    vertexBuffer.Init(allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    uploadContext.UploadBuffer(allocator, vertexBuffer, vertices.data(), bufferSize, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

    vertexCount = static_cast<uint32_t>(vertices.size());
}