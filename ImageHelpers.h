#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include "DeviceHandler.h"
#include "BufferHelpers.h"

namespace ImageHelpers {
    void CreateImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory,
        DeviceHandler*& deviceHandler
    ){
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; //optional

        if(vkCreateImage(deviceHandler->getLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) throw std::runtime_error("Failed to create VkImage.\n");

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(deviceHandler->getLogicalDevice(), image, &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = BufferHelpers::FindMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceHandler);

        if(vkAllocateMemory(deviceHandler->getLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) throw std::runtime_error("Failed to allocate image memory!\n");
        vkBindImageMemory(deviceHandler->getLogicalDevice(), image, imageMemory, 0);
    }

}