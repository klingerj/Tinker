#include "Platform/Graphics/VulkanTypes.h"
#include "Platform/Graphics/GPUMemAllocator.h"
#include "Core/Utilities/Logging.h"

#include <cstring>

namespace Tinker
{
namespace Platform
{
namespace Graphics
{

void AllocGPUMemory(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceMemory* deviceMemory,
    VkMemoryRequirements memRequirements, VkMemoryPropertyFlags memPropertyFlags)
{
    // Check for memory type support
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    uint32 memTypeIndex = 0xffffffff;
    for (uint32 uiMemType = 0; uiMemType < memProperties.memoryTypeCount; ++uiMemType)
    {
        if (((1 << uiMemType) & memRequirements.memoryTypeBits) &&
            (memProperties.memoryTypes[uiMemType].propertyFlags & memPropertyFlags) == memPropertyFlags)
        {
            memTypeIndex = uiMemType;
        }
    }
    if (memTypeIndex == 0xffffffff)
    {
        Core::Utility::LogMsg("Platform", "Failed to find memory property flags!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memRequirements.size;
    memAllocInfo.memoryTypeIndex = memTypeIndex;
    VkResult result = vkAllocateMemory(device, &memAllocInfo, nullptr, deviceMemory);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to allocate gpu memory!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, uint32 sizeInBytes,
    VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
    VkBuffer& buffer, VkDeviceMemory& deviceMemory)
{
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeInBytes;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to allocate Vulkan buffer!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    AllocGPUMemory(physicalDevice, device, &deviceMemory, memRequirements, propertyFlags);

    vkBindBufferMemory(device, buffer, deviceMemory, 0);
}

void CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectMask, VkImage image, VkImageView* imageView)
{
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, imageView);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan image view!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

void CreateFramebuffer(VkDevice device, VkImageView* colorRTs, uint32 numColorRTs, VkImageView depthRT,
    uint32 width, uint32 height, VkRenderPass renderPass, VkFramebuffer* framebuffer)
{
    VkImageView* attachments = nullptr;
    uint32 numAttachments = numColorRTs + (depthRT != VK_NULL_HANDLE ? 1 : 0);

    if (numAttachments == 0)
    {
        Core::Utility::LogMsg("Platform", "No attachments specified for framebuffer!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    else
    {
        attachments = new VkImageView[numAttachments];
    }

    if (attachments)
    {
        memcpy(attachments, colorRTs, sizeof(VkImageView) * numColorRTs); // memcpy the color render targets in

        if (depthRT != VK_NULL_HANDLE)
        {
            attachments[numAttachments - 1] = depthRT;
        }
    }

    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.attachmentCount = numAttachments;
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, framebuffer);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan framebuffer!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    delete attachments;
}

void CreateRenderPass(VkDevice device, uint32 numColorAttachments, VkFormat colorFormat,
    VkImageLayout startLayout, VkImageLayout endLayout, VkFormat depthFormat, VkRenderPass* renderPass)
{
    VkAttachmentDescription attachments[VULKAN_MAX_RENDERTARGETS_WITH_DEPTH] = {};
    VkAttachmentReference colorAttachmentRefs[VULKAN_MAX_RENDERTARGETS] = {};
    VkAttachmentReference depthAttachmentRef = {};

    for (uint32 uiRT = 0; uiRT < numColorAttachments; ++uiRT)
    {
        attachments[uiRT] = {};
        attachments[uiRT].format = colorFormat;
        attachments[uiRT].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[uiRT].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[uiRT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[uiRT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[uiRT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[uiRT].initialLayout = startLayout;
        attachments[uiRT].finalLayout = endLayout;

        colorAttachmentRefs[uiRT] = {};
        colorAttachmentRefs[uiRT].attachment = uiRT;
        colorAttachmentRefs[uiRT].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    bool hasDepthAttachment = depthFormat != VK_FORMAT_UNDEFINED;

    attachments[numColorAttachments].format = depthFormat;
    attachments[numColorAttachments].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[numColorAttachments].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[numColorAttachments].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[numColorAttachments].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[numColorAttachments].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[numColorAttachments].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[numColorAttachments].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    depthAttachmentRef.attachment = numColorAttachments;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = numColorAttachments;
    subpass.pColorAttachments = colorAttachmentRefs;
    subpass.pDepthStencilAttachment = hasDepthAttachment ? &depthAttachmentRef : nullptr;

    const uint32 numSubpassDependencies = 2;
    VkSubpassDependency subpassDependencies[numSubpassDependencies] = {};
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependencies[0].srcAccessMask = 0;
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | (hasDepthAttachment ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);

    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | (hasDepthAttachment ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependencies[1].dstAccessMask = 0;
    // TODO: check if these subpass dependencies are correct at some point

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = hasDepthAttachment ? numColorAttachments + 1 : numColorAttachments;
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = numSubpassDependencies;
    renderPassCreateInfo.pDependencies = subpassDependencies;

    VkResult result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, renderPass);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan render pass!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

}
}
}
