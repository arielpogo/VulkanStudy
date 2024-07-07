#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <optional>
#include <cstdint>

#include "SurfaceHandler.h"

struct QueueFamilyIndices{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily; //the queue that supports drawing and presenting may vary. You can create logic to prefer devices that have this as one queue for better performance

	bool isComplete(){
		return graphicsFamily.has_value() && presentFamily.has_value();
	}

	QueueFamilyIndices(){

	}

	QueueFamilyIndices(QueueFamilyIndices& other){
		graphicsFamily = other.graphicsFamily;
		presentFamily = other.presentFamily;
	}

	QueueFamilyIndices(const VkPhysicalDevice& device, SurfaceHandler* surfaceHandler){		
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for(const auto& queueFamily : queueFamilies){
			if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFamily = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surfaceHandler->getSurface(), &presentSupport);
			if(presentSupport) presentFamily = i;
			
			if(isComplete()) break;
			++i;
		}
	}
};