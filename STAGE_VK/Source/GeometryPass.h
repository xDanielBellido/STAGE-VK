/*
    ===========================================================================
    GeometryPass
    ===========================================================================

    Low-level rendering pass responsible for scene geometry.

    GeometryPass does not own the source of the geometry. Procedural geometry,
    imported models and other future mesh sources will eventually provide
    renderable geometry to this pass.

    Current responsibilities:
    - create and own the triangle vertex buffer
    - upload the initial vertex data through VulkanUploadContext
    - release owned geometry resources during shutdown
*/

#pragma once

#include "VulkanBuffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>

class VulkanAllocator;
class VulkanUploadContext;

class GeometryPass
{
private:

    struct Vertex
    {
        float x;
        float y;
        float z;
    };

    VkDevice            device = VK_NULL_HANDLE;

    VulkanBuffer        vertexBuffer;
    uint32_t            vertexCount = 0;

    VkPipelineLayout    pipelineLayout = VK_NULL_HANDLE;
    VkPipeline          graphicsPipeline = VK_NULL_HANDLE;

public:

                        // Lifetime
    bool                Init(VkDevice device, VkRenderPass renderPass, const VulkanAllocator& allocator, VulkanUploadContext& uploadContext);
    void                CleanUp();

private:

                        // Geometry Resources
    void                CreateTriangleVertexBuffer(const VulkanAllocator& allocator, VulkanUploadContext& uploadContext);

                        // Graphics Pipeline
    bool                CreateGraphicsPipeline(VkRenderPass renderPass);
};