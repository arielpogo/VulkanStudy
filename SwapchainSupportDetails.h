#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <vector>
#include <SurfaceHandler.h>

struct SwapchainSupportDetails{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

private:
	SurfaceHandler* surfaceHandler;

	void GetSwapchainSupportDetails(VkPhysicalDevice& device){
		//basic surface capabilities (min/max number of images in the swap chain, min/max width, height of images etc.)
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surfaceHandler->getSurface(), &capabilities);

		uint32_t surfaceFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surfaceHandler->getSurface(), &surfaceFormatCount, nullptr);

		if(surfaceFormatCount != 0){
			formats.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surfaceHandler->getSurface(), &surfaceFormatCount, formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surfaceHandler->getSurface(), &presentModeCount, nullptr);

		if(presentModeCount != 0){
			presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surfaceHandler->getSurface(), &presentModeCount, presentModes.data());
		}
	}

public:
	void Update(VkPhysicalDevice& device){
		GetSwapchainSupportDetails(device);
	}

	SwapchainSupportDetails(SwapchainSupportDetails& other){
		capabilities = other.capabilities;
		formats = other.formats;
		presentModes = other.presentModes;
		surfaceHandler = other.surfaceHandler;
	}

	SwapchainSupportDetails(VkPhysicalDevice& device, SurfaceHandler* _surfaceHandler) : surfaceHandler(_surfaceHandler){
		GetSwapchainSupportDetails(device);
	}
};