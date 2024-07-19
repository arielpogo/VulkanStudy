#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include "DeviceHandler.h"
#include "ImageHelpers.h"
#include "SwapchainHandler.h"

class DepthResourcesHandler{
    friend SwapchainHandler;
    
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    DeviceHandler* deviceHandler;

public:
    DepthResourcesHandler(DeviceHandler*& _dh, VkExtent2D swapchainExtent) : deviceHandler(_dh){
        createDepthResources(swapchainExtent);
    }

    ~DepthResourcesHandler(){
        vkDestroyImageView(deviceHandler->getLogicalDevice(), depthImageView, nullptr);
        vkDestroyImage(deviceHandler->getLogicalDevice(), depthImage, nullptr);
        vkFreeMemory(deviceHandler->getLogicalDevice(), depthImageMemory, nullptr);
    }

    inline VkImageView& getDepthImageView() {return depthImageView; }

protected:
    void createDepthResources(VkExtent2D swapchainExtent) {
        VkFormat depthFormat = findDepthFormat();

        ImageHelpers::CreateImage(swapchainExtent.width, swapchainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, deviceHandler);
        depthImageView = ImageHelpers::CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, deviceHandler->getLogicalDevice());
    }    

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(deviceHandler->getPhysicalDevice(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported depth format!");
    }

    inline VkFormat findDepthFormat() {
        return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }
};