#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <vector>
#include "DeviceHandler.h"
#include "Globals.h"

class CommandBuffersHandler{
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

    DeviceHandler* deviceHandler;
public:
    CommandBuffersHandler(DeviceHandler*& _dh) : deviceHandler(_dh){
        createCommandPool();
        createCommandBuffers();
    }

    ~CommandBuffersHandler(){
        vkDestroyCommandPool(deviceHandler->getLogicalDevice(), commandPool, nullptr); //command buffers automatically cleaned here too
    }

    inline std::vector<VkCommandBuffer>& GetCommandBuffers(){ return commandBuffers; }
    inline VkCommandPool& GetCommandPool(){ return commandPool; }

    VkCommandBuffer beginSingleTimeCommands() {
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

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(deviceHandler->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(deviceHandler->getGraphicsQueue());

        vkFreeCommandBuffers(deviceHandler->getLogicalDevice(), commandPool, 1, &commandBuffer);
    }

private:
    void createCommandPool(){
        QueueFamilyIndices& queueFamilyIndices = deviceHandler->getQueueFamilyIndices();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //command buffers rerecorded individually, rather than together (default)
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if(vkCreateCommandPool(deviceHandler->getLogicalDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) throw std::runtime_error("Failed to create command pool.\n");
    }

    void createCommandBuffers(){
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

		if(vkAllocateCommandBuffers(deviceHandler->getLogicalDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) throw std::runtime_error("Could not allocate command buffers.\n");
	}
};