#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <glm/glm.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>

#include <cstdint>

#include <WindowHandler.h>
#include <InstanceHandler.h>
#include <Vertex.h>

#define DEBUG 1

const int MAX_FRAMES_IN_FLIGHT = 2;
uint32_t currentFrame = 0;

static std::vector<char> binFileToByteVector(const std::string& filename){
	std::ifstream file(filename, std::ios::ate | std::ios::binary); //ate -> start at end -> to see read pos -> to det size of file -> to allocate a buffer

	if(!file.is_open()) throw std::runtime_error("Failed to open binary file.");

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

struct QueueFamilyIndices{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily; //the queue that supports drawing and presenting may vary. You can create logic to prefer devices that have this as one queue for better performance

	bool isComplete(){
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapchainSupportDetails{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

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

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
	
private:
	WindowHandler* windowHandler;
	InstanceHandler* instanceHandler;
	VkSurfaceKHR surface;
	
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device; //logical device opposed to physical device
	
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages; //auto cleaned by destroying the swapchain
	std::vector<VkImageView> swapchainImageViews; //views = how to "view" an image
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

	VkRenderPass renderPass;

    VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	std::vector<VkFramebuffer> swapchainFramebuffers;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

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

		vkDeviceWaitIdle(device); //prevent premature closure while the device is finishing up
	}

	void cleanup(){
		cleanupSwapchain();
		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){	
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);
		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);
		vkDestroyCommandPool(device, commandPool, nullptr); //command buffers automatically cleaned here too
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroyDevice(device, nullptr); //physical dev. handler is implicitly deleted, no need to do anything
		vkDestroySurfaceKHR(instanceHandler->getInstance(), surface, nullptr); //surface must be deleted before the instance
		delete windowHandler;
		glfwTerminate();
	}

	void initVulkan(){
		instanceHandler = new InstanceHandler(validationLayers);

		createSurface(); //influences what device may get picked
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createImageViews();
		createRenderPass();
        createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createVertexBuffer();
		createIndexBuffer();
		createCommandBuffers();
		createSyncObjects();
	}	

	void createSurface(){
		if(glfwCreateWindowSurface(instanceHandler->getInstance(), windowHandler->getWindowPointer(), nullptr, &surface) != VK_SUCCESS) throw std::runtime_error("Failed to create window surface\n");
	}

	void pickPhysicalDevice(){
		uint32_t deviceCount = 0;

		//again, just count
		vkEnumeratePhysicalDevices(instanceHandler->getInstance(), &deviceCount, nullptr);
	
		if(deviceCount == 0) throw std::runtime_error("Failed to find GPUs with Vulkan support\n");

		std::vector<VkPhysicalDevice> devices(deviceCount);

		//enumerate all available devices into the vector
		vkEnumeratePhysicalDevices(instanceHandler->getInstance(), &deviceCount, devices.data());

		//pick first suitable device
		for(const auto& d : devices){
			if(isDeviceSuitable(d)){
				physicalDevice = d;
				break;
			}
		}

		if(physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("Failed to find a suitable GPU\n");
	}

	bool isDeviceSuitable(VkPhysicalDevice device){
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		//find and get all indices of required queue families
		QueueFamilyIndices indices = findQueueFamilies(device);
		bool allExtensionsSupported = checkDeviceExtensionSupport(device);

		bool swapchainAdequate = false;
		if(allExtensionsSupported){
			SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device);
			swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
		}

		return indices.isComplete() && allExtensionsSupported && swapchainAdequate;
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

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device){
		QueueFamilyIndices indices;
			
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for(const auto& queueFamily : queueFamilies){
			if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if(presentSupport) indices.presentFamily = i;
			
			if(indices.isComplete()) break;
			++i;
		}
		
		return indices;
	}
	
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device){
		SwapchainSupportDetails details;
		
		//basic surface capabilities (min/max number of images in the swap chain, min/max width, height of images etc.)
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t surfaceFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, nullptr);

		if(surfaceFormatCount != 0){
			details.formats.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, details.formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if(presentModeCount != 0){
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	void createLogicalDevice(){
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		//queues to be created
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
		std::set<uint32_t> uniqueQueueFamilies = {
			indices.graphicsFamily.value(),
			indices.presentFamily.value()
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
		
		if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) throw std::runtime_error("Failed to create logical device\n");

		//get a handle to the created queues
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void createSwapchain(){
		SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

		//+1 because we don't want to have to wait on the driver to do internal operations before acquiring another image to render to
		uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

		if(swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) imageCount = swapchainSupport.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; //>1 for stereoscopic 3D images
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //directly render to images (different bits, such as VK_IMAGE_USAGE_TRANSFER_DST_BIT would be used for postprocessing, for example)
		
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
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

		if(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) throw std::runtime_error("Failed to create swapchain.\n");

		vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
		swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

		swapchainImageFormat = surfaceFormat.format;
		swapchainExtent = extent;
	}

	void cleanupSwapchain(){
		for(auto framebuffer : swapchainFramebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
		for(auto imageView : swapchainImageViews) vkDestroyImageView(device, imageView, nullptr);
		vkDestroySwapchainKHR(device, swapchain, nullptr); //swapchain must be deleted before the surface
	}

	void recreateSwapchain(){
		int width = 0, height = 0;
		glfwGetFramebufferSize(windowHandler->getWindowPointer(), &width, &height);

		while(width == 0 || height == 0){ //minimized app
			glfwGetFramebufferSize(windowHandler->getWindowPointer(), &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		cleanupSwapchain();
		createSwapchain();
		createImageViews();
		createFramebuffers();
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
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapchainImages[i];

			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapchainImageFormat;

			//default color swizzling (do nothing)
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange. layerCount = 1;

			if(vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) throw std::runtime_error("Failed to create image views.\n");
		}
	}

	void createRenderPass(){
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		//what to do with the data in the attachment before and after rendering
		//options for load op:
		//LOAD_OP_LOAD: preserve the existing contents of the attachment
		//LOAD_OP_CLEAR: clear the value to a constant at the start
		//LOAD_OP_DONT_CARE: existing conents are undefined, we dont care about them
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		//options for store op:
		//STORE_OP_STORE: rendered contents will be stored in memory and can be read later
		//STORE_OP_DONT_CARE: conents of the frambuffer will be undefined after the rendering operation
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		//we dont do anything with stencils so we dont care
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//image layout before the render pass begins
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //we dont care what the previous was
		//image layout to automatically convert to after the render pass
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0; //which attachment to reference to by index below
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef; //referenced directly from fragment shader with layout(location = 0)out vec4 outColor;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) throw std::runtime_error("Failed to create render pass!\n");
	}

    void createDescriptorSetLayout(){
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; //optional

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) throw std::runtime_error("Failed to create descriptor set layout.\n");
    }

	void createGraphicsPipeline(){
		std::vector<char> vertShaderCode = binFileToByteVector("shaders/vert.spv");
		std::vector<char> fragShaderCode = binFileToByteVector("shaders/frag.spv");
		if(DEBUG) std::cout << "vert: " << vertShaderCode.size() << " bytes frag: " << fragShaderCode.size() << " bytes\n";

		//these are only needed at graphics pipeline creation time, so are destroyed at the end of this function
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main"; //entrypoint name. We can combine multiple shaders into one module and change this value for whatever we need to, if we wanted to
		//vertShaderStageInfo.pSpecializationInfo can be used to set constants at pipeline creation rather than at render time. Optional to set.

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		//info about the vertex data provided
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; //optional
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); //optional

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) swapchainExtent.width;
		viewport.height = (float) swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		//scissor = where pixels are actually stored (filters out what is not needed)
		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapchainExtent;

		//things about the render pipeline we want control over during runtime (most everything else is baked and immutable)
		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //triangles from every 3 verticies without reuse
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		//if you want it to be static:
		//viewportState.pViewports = &viewport;
		//viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //or wireframe, etc., req. enabling a gpu feature
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //cull back faces
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; //vertex order to det. front and back
		rasterizer.depthBiasEnable = VK_FALSE;
		//optional
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		//optional
		multisampling.minSampleShading = 1.0f; 
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		//optional
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		//optional
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		//optional
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) throw std::runtime_error("Failed to create pipeline layout.\n");
		
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; //optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		//optional, for when you're copying an existing pipeline to modify it
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) throw std::runtime_error("Failed to create graphics pipeline.\n");

		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	VkShaderModule createShaderModule(const std::vector<char>& code){
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); //data is aligned in worst case scenario in an std::vector, we're good

		VkShaderModule shaderModule;
		if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) throw std::runtime_error("Failed to create shader module.\n");

		return shaderModule;
	}

	void createFramebuffers(){
		swapchainFramebuffers.resize(swapchainImageViews.size());

		for(size_t i = 0; i < swapchainImageViews.size(); ++i){
			VkImageView attachments[] = {swapchainImageViews[i]};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapchainExtent.width;
			framebufferInfo.height = swapchainExtent.height;
			framebufferInfo.layers = 1;

			if(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) throw std::runtime_error("Failed to create framebuffer!\n");
		}
	}

	void createCommandPool(){
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //command buffers rerecorded individually, rather than together (default)
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) throw std::runtime_error("Failed to create command pool.\n");
	}

	void createVertexBuffer(){
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t) bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createIndexBuffer(){
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t) bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
		
		copyBuffer(stagingBuffer, indexBuffer, bufferSize);
		
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory){
		VkBufferCreateInfo bufferInfo{};	
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) throw std::runtime_error("Failed to create buffer.\n");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) throw std::runtime_error("Failed to allocate buffer memory.\n");
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; //optional
		copyRegion.dstOffset = 0; //optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties){
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for(int32_t i = 0; i < memProperties.memoryTypeCount; ++i){
			if((typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)) return i;
		}

		throw std::runtime_error("Failed to find suitable memory type.\n");
	}

	void createCommandBuffers(){
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

		if(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) throw std::runtime_error("Could not allocate command buffers.\n");
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
		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
			if(
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
			) throw std::runtime_error ("Failed to create semaphores.\n");
		}
		
	}

	void drawFrame(){
		//steps to draw a frame are outlined with the comments:
		//wait for previous frame to finish
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		//acquire an image from the swap chain
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if(result == VK_ERROR_OUT_OF_DATE_KHR){
			recreateSwapchain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) throw std::runtime_error("Failed to acquire swapchain image!\n");

		vkResetFences(device, 1, &inFlightFences[currentFrame]); //only reset the fence if we are submitting work

		//record a command buffer which draws the scene onto that image
		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		//submit the recorded command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) throw std::runtime_error("Failed to submit draw command buffer.\n");

		//present the swapchain image
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapchains[] = {swapchain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; //optional

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			recreateSwapchain();
			framebufferResized = false;
		}
		else if (result != VK_SUCCESS) throw std::runtime_error("Failed to present swapchain image\n");

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
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchainExtent;
		
		VkClearValue clearColor  = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		//to commandBuffer, record a BeginRenderPass command, using &renderPassInfo, into a primary command buffer
		//all vkCmd functions return void; error handling is done after recording
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		//these are the dynamic state things specified when creating the pipeline:
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchainExtent.width);
		viewport.height = static_cast<float>(swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapchainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) throw std::runtime_error("Failed to record command buffer!\n");
	}
};

static void framebufferResizeCallback(GLFWwindow* window, int width, int height){
    Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->framebufferResized = true; //let the renderer know the framebuffer was resized
}