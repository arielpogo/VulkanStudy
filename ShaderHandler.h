#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <vector>
#include <fstream>
#include <iostream>
#include "Globals.h"

class ShaderHandler{
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    VkDevice& logicalDevice;
    
public:
    ShaderHandler(std::string vertShaderPath, std::string fragShaderPath, VkDevice& _ld) : logicalDevice(_ld){
        createShaderModule(vertShaderPath, vertShaderModule);
        createShaderModule(fragShaderPath, fragShaderModule);
    }

    ~ShaderHandler(){
        vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
    }

    inline VkShaderModule& getVertShaderModule() { return vertShaderModule; }
    inline VkShaderModule& getFragShaderModule() { return fragShaderModule; }

private:
    void createShaderModule(std::string& filename, VkShaderModule& shaderModule){
        std::ifstream file(filename, std::ios::ate | std::ios::binary); //ate -> start at end -> to see read pos -> to det size of file -> to allocate a buffer

        if(!file.is_open()) throw std::runtime_error("Failed to open binary file.");

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> code(fileSize);

        file.seekg(0);
        file.read(code.data(), fileSize);

        file.close();
        
        if(DEBUG) std::cout << '"' << filename << "\" loaded as: " << code.size() << " bytes\n";

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); //data is aligned in worst case scenario in an std::vector, we're good

		if(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) throw std::runtime_error("Failed to create shader module.\n");
	}
};