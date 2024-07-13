#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include "DeviceHandler.h"
#include "CommandBuffersHandler.h"

namespace BufferHelpers {
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, DeviceHandler*& deviceHandler){
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(deviceHandler->getPhysicalDevice(), &memProperties);

		for(int32_t i = 0; i < memProperties.memoryTypeCount; ++i){
			if((typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)) return i;
		}

		throw std::runtime_error("Failed to find suitable memory type.\n");
	}

    void CreateBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer& buffer,
                      VkDeviceMemory& bufferMemory,
                      DeviceHandler*& deviceHandler){
        VkBufferCreateInfo bufferInfo{};	
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(deviceHandler->getLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) throw std::runtime_error("Failed to create buffer.\n");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(deviceHandler->getLogicalDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = BufferHelpers::FindMemoryType(memRequirements.memoryTypeBits, properties, deviceHandler);

        if(vkAllocateMemory(deviceHandler->getLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) throw std::runtime_error("Failed to allocate buffer memory.\n");
        vkBindBufferMemory(deviceHandler->getLogicalDevice(), buffer, bufferMemory, 0);
    }

    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, CommandBuffersHandler*& buffersHandler) {
        VkCommandBuffer commandBuffer = buffersHandler->beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        buffersHandler->endSingleTimeCommands(commandBuffer);
    }
}