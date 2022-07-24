#include "VulkanTypes.h"
#include "Utility/Logging.h"

#include <cstring>

namespace Tk
{
namespace Core
{
namespace Graphics
{

VulkanContextResources g_vulkanContextResources = {};

// NOTE: Must correspond the enums in PlatformGameAPI.h
static VkPipelineColorBlendAttachmentState   VulkanBlendStates    [BlendState::eMax]     = {};
static VkPipelineDepthStencilStateCreateInfo VulkanDepthStates    [DepthState::eMax]     = {};
static VkImageLayout                         VulkanImageLayouts   [ImageLayout::eMax]    = {};
static VkFormat                              VulkanImageFormats   [ImageFormat::eMax]    = {};
static VkDescriptorType                      VulkanDescriptorTypes[DescriptorType::eMax] = {};

void InitVulkanDataTypesPerEnum()
{
    VkPipelineColorBlendAttachmentState blendStateProperties = {};
    blendStateProperties.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    blendStateProperties.blendEnable = VK_TRUE;
    blendStateProperties.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendStateProperties.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendStateProperties.colorBlendOp = VK_BLEND_OP_ADD;
    blendStateProperties.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendStateProperties.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendStateProperties.alphaBlendOp = VK_BLEND_OP_ADD;
    VulkanBlendStates[BlendState::eAlphaBlend] = blendStateProperties;
    blendStateProperties.blendEnable = VK_FALSE;
    VulkanBlendStates[BlendState::eReplace] = blendStateProperties;
    VulkanBlendStates[BlendState::eNoColorAttachment] = {};

    VkCompareOp depthCompareOps[DepthCompareOp::eMax] = {};
    depthCompareOps[DepthCompareOp::eLeOrEqual] = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthCompareOps[DepthCompareOp::eGeOrEqual] = VK_COMPARE_OP_GREATER_OR_EQUAL;
    TINKER_ASSERT(DEPTH_OP < ARRAYCOUNT(depthCompareOps));

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthCompareOp = depthCompareOps[DEPTH_OP];
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.minDepthBounds = DEPTH_MIN;
    depthStencilState.maxDepthBounds = DEPTH_MAX;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = {};
    depthStencilState.back = {};

    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;
    VulkanDepthStates[DepthState::eOff] = depthStencilState;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    VulkanDepthStates[DepthState::eTestOnWriteOn] = depthStencilState;
    depthStencilState.depthWriteEnable = VK_FALSE;
    VulkanDepthStates[DepthState::eTestOnWriteOff] = depthStencilState;

    VulkanImageLayouts[ImageLayout::eUndefined] = VK_IMAGE_LAYOUT_UNDEFINED;
    VulkanImageLayouts[ImageLayout::eShaderRead] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VulkanImageLayouts[ImageLayout::eTransferDst] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    VulkanImageLayouts[ImageLayout::eDepthOptimal] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VulkanImageLayouts[ImageLayout::eRenderOptimal] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VulkanImageLayouts[ImageLayout::ePresent] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VulkanImageFormats[ImageFormat::Invalid] = VK_FORMAT_UNDEFINED;
    VulkanImageFormats[ImageFormat::BGRA8_SRGB] = VK_FORMAT_B8G8R8A8_SRGB;
    VulkanImageFormats[ImageFormat::RGBA8_SRGB] = VK_FORMAT_R8G8B8A8_SRGB;
    VulkanImageFormats[ImageFormat::Depth_32F] = VK_FORMAT_D32_SFLOAT;
    VulkanImageFormats[ImageFormat::TheSwapChainFormat] = g_vulkanContextResources.swapChainFormat;

    VulkanDescriptorTypes[DescriptorType::eBuffer] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VulkanDescriptorTypes[DescriptorType::eSampledImage] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VulkanDescriptorTypes[DescriptorType::eSSBO] = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
}

const VkPipelineColorBlendAttachmentState& GetVkBlendState(uint32 gameBlendState)
{
    TINKER_ASSERT(gameBlendState < BlendState::eMax);
    return VulkanBlendStates[gameBlendState];
}

const VkPipelineDepthStencilStateCreateInfo& GetVkDepthState(uint32 gameDepthState)
{
    TINKER_ASSERT(gameDepthState < DepthState::eMax);
    return VulkanDepthStates[gameDepthState];
}

const VkImageLayout& GetVkImageLayout(uint32 gameImageLayout)
{
    TINKER_ASSERT(gameImageLayout < ImageLayout::eMax);
    return VulkanImageLayouts[gameImageLayout];
}

const VkFormat& GetVkImageFormat(uint32 gameImageFormat)
{
    TINKER_ASSERT(gameImageFormat < ImageFormat::eMax);
    return VulkanImageFormats[gameImageFormat];
}

const VkDescriptorType& GetVkDescriptorType(uint32 gameDescriptorType)
{
    TINKER_ASSERT(gameDescriptorType < DescriptorType::eMax);
    return VulkanDescriptorTypes[gameDescriptorType];
}

}
}
}
