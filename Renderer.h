#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <limits>
#include <algorithm>
#include <fstream>
#include <chrono>

#include <cstdint>

#include "Globals.h"

#include "QueueFamilyIndices.h"
#include "SwapchainSupportDetails.h"

#include "WindowHandler.h"
#include "InstanceHandler.h"
#include "SurfaceHandler.h"
#include "DeviceHandler.h"
#include "SwapchainHandler.h"
#include "Vertex.h"
#include "UniformBuffers.h"
#include "TextureHandler.h"
#include "BufferHelpers.h"
#include "DescriptorSetsHandler.h"
#include "GraphicsPipelineHandler.h"
#include "CommandBuffersHandler.h"
#include "DepthResourcesHandler.h"
#include "ModelHandler.h"

uint32_t currentFrame = 0;

static void framebufferResizeCallback(GLFWwindow*, int, int);

class Renderer{
public:
	bool framebufferResized = false;

	void run(){
		windowHandler = new WindowHandler();
        glfwSetWindowUserPointer(windowHandler->getWindowPointer(), this);
		glfwSetFramebufferSizeCallback(windowHandler->getWindowPointer(), framebufferResizeCallback);

		initVulkan();
		mainLoop();
		cleanup();	
	}
	
private:
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
	
	WindowHandler* windowHandler;
	InstanceHandler* instanceHandler;
	SurfaceHandler* surfaceHandler;
    DeviceHandler* deviceHandler;
	SwapchainHandler* swapchainHandler;
	RenderPassHandler* renderPassHandler;
	UniformBuffers* uniformBuffers;
	DescriptorSetsHandler* descriptorSets;
	GraphicsPipelineHandler* graphicsPipelineHandler;
	CommandBuffersHandler* commandBuffersHandler;

	TextureHandler* texture;
	ModelHandler* model;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores; //
	std::vector<VkFence> inFlightFences; //used to block host while gpu is rendering the previous frame

	void mainLoop(){
		while(!glfwWindowShouldClose(windowHandler->getWindowPointer())){
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(deviceHandler->getLogicalDevice()); //prevent premature closure while the device is finishing up
	}

	void initVulkan(){
		instanceHandler = new InstanceHandler(validationLayers);
		surfaceHandler = new SurfaceHandler(instanceHandler, windowHandler);
        deviceHandler = new DeviceHandler(instanceHandler, surfaceHandler, validationLayers);

		VkDevice& logicalDevice = deviceHandler->getLogicalDevice();
		
		commandBuffersHandler = new CommandBuffersHandler(deviceHandler);
		texture = new TextureHandler(TEXTURE_PATH, deviceHandler, commandBuffersHandler);
		model = new ModelHandler(MODEL_PATH);
		
        swapchainHandler = new SwapchainHandler(windowHandler, surfaceHandler, deviceHandler);
		renderPassHandler = new RenderPassHandler(logicalDevice, swapchainHandler->getSwapchainImageFormat(), swapchainHandler->findDepthFormat());
		uniformBuffers = new UniformBuffers(deviceHandler, swapchainHandler);
		descriptorSets = new DescriptorSetsHandler(logicalDevice, uniformBuffers, texture);
		graphicsPipelineHandler = new GraphicsPipelineHandler(logicalDevice, swapchainHandler, descriptorSets->getDescriptorSetLayout(), renderPassHandler->getRenderPass());
		swapchainHandler->createInitialFrameBuffers(renderPassHandler);

		createVertexBuffer();
		createIndexBuffer();
		createSyncObjects();

		if(DEBUG) std::cout << "Vulkan Successfully Initialized.\n";		
	}

	void cleanup(){
		VkDevice& device = deviceHandler->getLogicalDevice();

		delete swapchainHandler;
		delete graphicsPipelineHandler;
		delete renderPassHandler;
		delete uniformBuffers;
		delete descriptorSets;
		delete model;
		delete texture;
		
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);
		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){	
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		delete commandBuffersHandler;
		delete deviceHandler;
		delete surfaceHandler; //surface must be deleted before the instance
        delete instanceHandler;
		delete windowHandler;

		glfwTerminate();
	}

	void createVertexBuffer(){
		VkDeviceSize bufferSize = sizeof(*model->getVertexData()) * model->getVertexDataSize();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		BufferHelpers::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, deviceHandler);

        VkDevice& device = deviceHandler->getLogicalDevice();

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, model->getVertexData(), (size_t) bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		BufferHelpers::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory, deviceHandler);

		BufferHelpers::CopyBuffer(stagingBuffer, vertexBuffer, bufferSize, commandBuffersHandler);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createIndexBuffer(){
		VkDeviceSize bufferSize = sizeof(*model->getIndicesData()) * model->getIndicesDataSize();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		BufferHelpers::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, deviceHandler);

        VkDevice& device = deviceHandler->getLogicalDevice();

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, model->getIndicesData(), (size_t) bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		BufferHelpers::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory, deviceHandler);
		BufferHelpers::CopyBuffer(stagingBuffer, indexBuffer, bufferSize, commandBuffersHandler);
		
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createSyncObjects(){
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //so the first frame isn't waiting for a previous (nonexistant) frame to finish rendering

        VkDevice& device = deviceHandler->getLogicalDevice();

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
			if(
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
			) throw std::runtime_error ("Failed to create semaphores.\n");
		}
		
	}

	void drawFrame() {
		VkDevice& device = deviceHandler->getLogicalDevice();

        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchainHandler->getSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            swapchainHandler->recreateSwapchain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        uniformBuffers->updateUniformBuffer(currentFrame);

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffersHandler->GetCommandBuffers()[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffersHandler->GetCommandBuffers()[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffersHandler->GetCommandBuffers()[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(deviceHandler->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapchainHandler->getSwapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(deviceHandler->getPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            swapchainHandler->recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex){
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//optional
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) throw std::runtime_error("Failed to beign recording command buffer.\n");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPassHandler->getRenderPass();
		renderPassInfo.framebuffer = swapchainHandler->getSwapchainFramebuffers()[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchainHandler->getSwapchainExtent();
		
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0] = {{0.0f, 0.0f, 0.0f, 1.0f}}; //color attachments
		clearValues[1] = {1.0f, 0}; //default the depth values at each pixel to the farthest depth away (far plane)

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		//to commandBuffer, record a BeginRenderPass command, using &renderPassInfo, into a primary command buffer
		//all vkCmd functions return void; error handling is done after recording
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineHandler->getGraphicsPipeline());

		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		//these are the dynamic state things specified when creating the pipeline:
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchainHandler->getSwapchainExtent().width);
		viewport.height = static_cast<float>(swapchainHandler->getSwapchainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapchainHandler->getSwapchainExtent();
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineHandler->getPipelineLayout(), 0, 1, &descriptorSets->getDescriptorSets()[currentFrame], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(model->getIndicesDataSize()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) throw std::runtime_error("Failed to record command buffer!\n");
	}
};

static void framebufferResizeCallback(GLFWwindow* window, int width, int height){
    Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->framebufferResized = true; //let the renderer know the framebuffer was resized
}