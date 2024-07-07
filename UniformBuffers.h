#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <glm/glm.hpp>
#include "Globals.h"
#include "DeviceHandler.h"
#include "BufferHelpers.h"
#include <chrono>

struct UniformBufferObject{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

class UniformBuffers{
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

    DeviceHandler* deviceHandler;
	SwapchainHandler* swapchainHandler;

public:
    UniformBuffers(DeviceHandler* _dh, SwapchainHandler* _sh) : deviceHandler(_dh), swapchainHandler(_sh){ //device handler needed for buffer helpers
        createUniformBuffers();
    }

    ~UniformBuffers(){
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
			vkDestroyBuffer(deviceHandler->getLogicalDevice(), uniformBuffers[i], nullptr);
			vkFreeMemory(deviceHandler->getLogicalDevice(), uniformBuffersMemory[i], nullptr);
		}
    }

	inline std::vector<VkBuffer>& getBuffers(){ return uniformBuffers; }

	void createUniformBuffers(){
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
			BufferHelpers::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i], deviceHandler);
			vkMapMemory(deviceHandler->getLogicalDevice(), uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void updateUniformBuffer(uint32_t currentImage){
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	
		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.projection = glm::perspective(glm::radians(45.0f), swapchainHandler->getSwapchainExtent().width / (float) swapchainHandler->getSwapchainExtent().height, 0.1f, 10.0f);
		ubo.projection[1][1] *= -1; //glm was originally for opengl which has the y clip coordinates inverted from Vulkan

		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}
};