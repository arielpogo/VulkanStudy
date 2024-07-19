#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

class SwapchainHandler; //forward decl for DepthResourcesHandler

#include <vector>
#include <algorithm>

#include "SwapchainSupportDetails.h"
#include "WindowHandler.h"
#include "DeviceHandler.h"
#include "SurfaceHandler.h"
#include "RenderPassHandler.h"
#include "ImageHelpers.h"
#include "DepthResourcesHandler.h"

class SwapchainHandler{
    VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages; //auto cleaned by destroying the swapchain
	std::vector<VkImageView> swapchainImageViews; //views = how to "view" an image
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

    std::vector<VkFramebuffer> swapchainFramebuffers;

	DepthResourcesHandler* depthResourcesHandler;

    WindowHandler* windowHandler;
    SurfaceHandler* surfaceHandler;
    DeviceHandler* deviceHandler;
    RenderPassHandler* renderPassHandler;
	
public:

    SwapchainHandler(WindowHandler* _wh, SurfaceHandler* _sh, DeviceHandler* _dh)
    : windowHandler(_wh), surfaceHandler(_sh), deviceHandler(_dh)
    {
        createSwapchain();
		createImageViews();
		depthResourcesHandler = new DepthResourcesHandler(deviceHandler, swapchainExtent);
    }

    ~SwapchainHandler(){
		delete depthResourcesHandler; //should happen right before swapchain cleaned
        cleanupSwapchain();
    }

    inline VkSwapchainKHR& getSwapchain() { return swapchain; }
    inline VkFormat& getSwapchainImageFormat() { return swapchainImageFormat; }
    inline VkExtent2D& getSwapchainExtent() { return swapchainExtent; }
    inline std::vector<VkFramebuffer>& getSwapchainFramebuffers(){ return swapchainFramebuffers; }
	inline VkFormat findDepthFormat(){ return depthResourcesHandler->findDepthFormat(); }

	void recreateSwapchain(){
		int width = 0, height = 0;
		glfwGetFramebufferSize(windowHandler->getWindowPointer(), &width, &height);

		while(width == 0 || height == 0){ //minimized app
			glfwGetFramebufferSize(windowHandler->getWindowPointer(), &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(deviceHandler->getLogicalDevice());

		cleanupSwapchain();
		createSwapchain();
		createImageViews();
		delete depthResourcesHandler;
		depthResourcesHandler = new DepthResourcesHandler(deviceHandler, swapchainExtent);
		createFramebuffers();
	}

    void cleanupSwapchain(){
        VkDevice& device = deviceHandler->getLogicalDevice();

		for(auto framebuffer : swapchainFramebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
		for(auto imageView : swapchainImageViews) vkDestroyImageView(device, imageView, nullptr);
		vkDestroySwapchainKHR(device, swapchain, nullptr); //swapchain must be deleted before the surface
	}

    void createInitialFrameBuffers(RenderPassHandler*& _rph){
		renderPassHandler = _rph;
		createFramebuffers();
    }

private:
    void createSwapchain(){
		SwapchainSupportDetails& swapchainSupport = deviceHandler->UpdateSwapchainSupportDetails();

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

		//+1 because we don't want to have to wait on the driver to do internal operations before acquiring another image to render to
		uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

		if(swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) imageCount = swapchainSupport.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surfaceHandler->getSurface();
		
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; //>1 for stereoscopic 3D images
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //directly render to images (different bits, such as VK_IMAGE_USAGE_TRANSFER_DST_BIT would be used for postprocessing, for example)
		
		QueueFamilyIndices indices = deviceHandler->getQueueFamilyIndices();
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		if(indices.graphicsFamily != indices.presentFamily){
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //multiple queues have to share an image simultaneously
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;

		}
		else{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //one queue has ownership at a time, must be explicitly changed (better performance)
			createInfo.queueFamilyIndexCount = 0; //optional
			createInfo.pQueueFamilyIndices = nullptr; //optional
		}
		
		createInfo.preTransform = swapchainSupport.capabilities.currentTransform; //do not flip the image by applying some transform
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //compositeAlpha is if alpha channel should be for blending with other windows in the windowing system. We don't want this, so we ignore it with the value it's set to
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE; //if another window blocks some pixels on the screen, don't render those pixels
		
		createInfo.oldSwapchain = VK_NULL_HANDLE; //if a swap chain becomes invalid/unoptimized and is completely recreated, the onld one must go here (for a complex reason)

        VkDevice& device = deviceHandler->getLogicalDevice();

		if(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) throw std::runtime_error("Failed to create swapchain.\n");

		vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
		swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

		swapchainImageFormat = surfaceFormat.format;
		swapchainExtent = extent;
	}

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats){
		//we want to search for B, G, R, A to be stored in an 8 bit unsigned int, and that SRGB is supported
		for(const auto& availableFormat : availableFormats){
			if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return availableFormat;
		}

		//failed, just use the first one specified
		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes){
		/* Types of present modes:
		 * IMMEDIATE_KHR - images are transferred right away, may result in tearing
		 * FIFO_KHR - top of queue is displayed when display refreshes, the program places new images at the back. If the queue is full, the program must wait. Most similar to "vsync"
		 * FIFO_RELAXED_KHR - differs from the previous only if the app is late and the queue was empty at the display refresh (display refresh is aka "vertical blank"), then when the image is ready it is immediately displayed instead of waiting for the next vertical blank. May cause tearing
		 * MAILBOX_KHR - variation of FIFO_KHR where instead of blocking the app when the queue is full, the images alread queued are replaced with more, newer images. So, frames are rendered as fast as possible without tearing, so less latency issues than vsync. AKA triple buffering, but this doesn't mean that the framerate is truly unlocked.
		 */
		
		//mailbox is a good tradeoff if energy use is not a concern. If it is (like in mobile devices), then FIFO_KHR is likely better
		/*for(const auto& availablePresentMode : availablePresentModes){
		if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return availablePresentMode;
		}*/


		//guaranteed to be available if anything
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	//swap extent == resolution
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities){
		//Vulkan tells us to match the resolution of the window by setting the "width" and "height" in the "currentExtent" member
		//However, some window managers allow us to differ here, so we can use the magic value of the max of uint32_t, so we can pick the resolution that best matches the window w/i minImageExtent and maxImageExtent bounds
		//But we must specify the resolution in the correct unit
		
		//GLFW uses two units for sizes: pixels and screen coordinates. Vulkan works with pixels. If you use high DPI displays s.a. Apple's retina displays, screen coordinates don't correspond to pixels, instead because of high pixel density, the resolution of the window in pixels will be large than in screen coordinates. 
		//So, we must use glfwGetFramebufferSize to get the res of the window in pixels before matching it against min/max
		
		if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;

		int width, height;
		glfwGetFramebufferSize(windowHandler->getWindowPointer(), &width, &height);
		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}

    void createImageViews(){
		swapchainImageViews.resize(swapchainImages.size());

		for(size_t i = 0; i < swapchainImages.size(); ++i){
			swapchainImageViews[i] = ImageHelpers::CreateImageView(swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, deviceHandler->getLogicalDevice());
		}
	}

	void createFramebuffers() {
        swapchainFramebuffers.resize(swapchainImageViews.size());

        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                swapchainImageViews[i],
                depthResourcesHandler->getDepthImageView()
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPassHandler->getRenderPass();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(deviceHandler->getLogicalDevice(), &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
};