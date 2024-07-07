#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <set>
#include <vector>
#include <cstdint>

#include "InstanceHandler.h"
#include "SurfaceHandler.h"
#include "QueueFamilyIndices.h"
#include "SwapchainSupportDetails.h"

class DeviceHandler{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice;

    QueueFamilyIndices* queueFamilyIndices;
    SwapchainSupportDetails* swapchainSupport;

    VkQueue graphicsQueue;
	VkQueue presentQueue;

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

public:
    inline VkPhysicalDevice& getPhysicalDevice() { return physicalDevice; }
    inline VkDevice& getLogicalDevice() { return logicalDevice; }

    inline QueueFamilyIndices& getQueueFamilyIndices(){ return *queueFamilyIndices; }
    inline SwapchainSupportDetails& getSwapchainSupportDetails(){ return *swapchainSupport; }

    inline VkQueue& getGraphicsQueue(){ return graphicsQueue; }
    inline VkQueue& getPresentQueue(){ return presentQueue; }

    SwapchainSupportDetails& UpdateSwapchainSupportDetails(){
        swapchainSupport->Update(physicalDevice);
        return getSwapchainSupportDetails();
    }

    DeviceHandler(InstanceHandler* instanceHandler, SurfaceHandler* surfaceHandler, const std::vector<const char*>& validationLayers){
        pickPhysicalDevice(instanceHandler, surfaceHandler);
        createLogicalDevice(validationLayers);
    }

    ~DeviceHandler(){
        vkDestroyDevice(logicalDevice, nullptr); //physical dev. handler is implicitly deleted, no need to do anything
        delete queueFamilyIndices;
        delete swapchainSupport;
    }

private:
    void pickPhysicalDevice(InstanceHandler* instanceHandler, SurfaceHandler* surfaceHandler){
        //just count
        uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instanceHandler->getInstance(), &deviceCount, nullptr);
		if(deviceCount == 0) throw std::runtime_error("Failed to find GPUs with Vulkan support\n");

		//enumerate all available devices into the vector
        std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instanceHandler->getInstance(), &deviceCount, devices.data());

		//pick first suitable device
		for(auto& d : devices){
			if(isDeviceSuitable(d, surfaceHandler)){
				physicalDevice = d;
				break;
			}
		}

		if(physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("Failed to find a suitable GPU\n");
    }

	bool isDeviceSuitable(VkPhysicalDevice& device, SurfaceHandler* surfaceHandler){
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		//find and get all indices of required queue families
		QueueFamilyIndices indices(device, surfaceHandler);
		bool allExtensionsSupported = checkDeviceExtensionSupport(device);

        if(!allExtensionsSupported) return false;

		bool swapchainAdequate = false;
		SwapchainSupportDetails support(device, surfaceHandler);
		swapchainAdequate = !support.formats.empty() && !support.presentModes.empty();

		if(indices.isComplete() && allExtensionsSupported && swapchainAdequate){
            queueFamilyIndices = new QueueFamilyIndices(indices);
            swapchainSupport = new SwapchainSupportDetails(support);
            return true;
        }
        else return false;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device){
		uint32_t extensionCount;

		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for(const auto& e : availableExtensions) requiredExtensions.erase(e.extensionName);

		return requiredExtensions.empty();
	}

    void createLogicalDevice(const std::vector<const char*>& validationLayers){
        //queues to be created
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
		std::set<uint32_t> uniqueQueueFamilies = {
			queueFamilyIndices->graphicsFamily.value(),
			queueFamilyIndices->presentFamily.value()
		};

		float queuePriority = 1.0f;
		for(uint32_t queueFamily : uniqueQueueFamilies){
			VkDeviceQueueCreateInfo queueCreateInfo{};

			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		//specifying what device features are needed
		VkPhysicalDeviceFeatures deviceFeatures{};

		//creating the logical device
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		//not needed anymore, but req. for older implementations
		if(enableValidationLayers){
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else{
			createInfo.enabledLayerCount = 0;
		}
		
		if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) throw std::runtime_error("Failed to create logical device\n");

		//get a handle to the created queues
		vkGetDeviceQueue(logicalDevice, queueFamilyIndices->graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, queueFamilyIndices->presentFamily.value(), 0, &presentQueue);
    }
};