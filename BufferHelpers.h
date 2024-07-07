#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include "DeviceHandler.h"

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

    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, DeviceHandler*& deviceHandler, VkCommandPool& commandPool){
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(deviceHandler->getLogicalDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; //optional
        copyRegion.dstOffset = 0; //optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(deviceHandler->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(deviceHandler->getGraphicsQueue());

        vkFreeCommandBuffers(deviceHandler->getLogicalDevice(), commandPool, 1, &commandBuffer);
    }
}