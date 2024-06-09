#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <InstanceHandler.h>
#include <WindowHandler.h>

class SurfaceHandler{
    VkSurfaceKHR surface;
    InstanceHandler* instanceHandler;

public:
    inline VkSurfaceKHR& getSurface() { return surface; }

    SurfaceHandler(InstanceHandler* _instanceHandler, WindowHandler* _windowHandler) : instanceHandler(_instanceHandler){
        if(glfwCreateWindowSurface(_instanceHandler->getInstance(), _windowHandler->getWindowPointer(), nullptr, &surface) != VK_SUCCESS) throw std::runtime_error("Failed to create window surface\n");
    }

    ~SurfaceHandler(){
        vkDestroySurfaceKHR(instanceHandler->getInstance(), surface, nullptr);
    }
};