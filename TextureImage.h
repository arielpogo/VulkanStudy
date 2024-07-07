#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image.h"
#include <stdexcept>

#include "ImageHelpers.h"
#include "DeviceHandler.h"

class TextureImage{
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;

    VkDevice& logicalDevice;

public:
    TextureImage(const char* path, DeviceHandler*& deviceHandler) : logicalDevice(deviceHandler->getLogicalDevice()){ //device handler needed for BufferHelpers
        int texWidth, texHeight, texChannels;
        
        stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if(!pixels) throw std::runtime_error("Failed to load texture.\n");
        VkDeviceSize imageSize = texWidth * texHeight * 4; //4 = num channels

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        BufferHelpers::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, deviceHandler);

        void* data;
        vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logicalDevice, stagingBufferMemory);

        stbi_image_free(pixels);

        ImageHelpers::CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, deviceHandler);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }

    ~TextureImage(){
        vkDestroyImage(logicalDevice, textureImage, nullptr);
        vkFreeMemory(logicalDevice, textureImageMemory, nullptr);
    }
};