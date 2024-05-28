#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>

#include <cstring>

#define DEBUG 1

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifndef DEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily; //the queue that supports drawing and presenting may vary. You can create logic to prefer devices that have this as one queue for better performance

	bool isComplete(){
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Application{
public:
	void run(){
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();	
	}
	
private:
	void initWindow(){
		glfwInit(); //init glfw library
		
		//do not create an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(HEIGHT, WIDTH, "Vulkan", nullptr, nullptr);
	}

	void initVulkan(){
		createInstance();
		createSurface(); //influences what device may get picked
		pickPhysicalDevice();
		createLogicalDevice();
	}

	void createInstance(){
		if(enableValidationLayers && !checkValidationLayerSupport()) throw std::runtime_error("Validation layer(s) requested, but not available\n");

		//technically optional, but provides useful optimization info to the driver
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "VulkanStudy";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		//not optional
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		//vulkan is platform agnostic, so an extension is needed to interact with the windowing system of the OS
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		//glfw handles this for us
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		//count all available extensions
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		
		//vector to hold the details of the extensions
		std::vector<VkExtensionProperties> extensions(extensionCount);
		//get all extensions (notice it's the same fn from before)
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::cout << "Available extensions:\n";
		for(const auto& e : extensions) std::cout << '\t' << e.extensionName << '\n';
			
		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		createInfo.enabledLayerCount = 0;

		if(enableValidationLayers){
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else createInfo.enabledLayerCount = 0;

		if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) throw std::runtime_error("Failed to create Vulkan instance!");
	}

	bool checkValidationLayerSupport(){
		uint32_t layerCount;

		//just count
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);

		//enumerate all available validation layers (into the vector)
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		std::cout << "Available validation layers:\n";
	       	for(const auto& availableLayer : availableLayers) std::cout << '\t' << availableLayer.layerName << '\n';

		//check if each validation layer we want is available
		bool found = false;
		for(const char* layerName : validationLayers){
			for(const auto& availableLayer : availableLayers){
				if(strcmp(layerName, availableLayer.layerName) == 0){
					found = true;
					break;
				}
			}
			if(!found) {
				std::cout << "Unable to find validation layer: " << layerName << '\n';
				return false;
			}
		}

		return true;
	}	

	void createSurface(){
		if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) throw std::runtime_error("Failed to create window surface\n");
	}

	void pickPhysicalDevice(){
		uint32_t deviceCount = 0;

		//again, just count
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	
		if(deviceCount == 0) throw std::runtime_error("Failed to find GPUs with Vulkan support\n");

		std::vector<VkPhysicalDevice> devices(deviceCount);

		//enumerate all available devices into the vector
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

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

		bool swapChainAdequate = false;
		if(allExtensionsSupported){
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && allExtensionsSupported && swapChainAdequate;
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
	
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device){
		SwapChainSupportDetails details;
		
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

	

	void mainLoop(){
		while(!glfwWindowShouldClose(window)){
			glfwPollEvents();
		}
	}

	void cleanup(){
		//physical dev. handler is implicitly deleted, no need to do anything

		vkDestroySurfaceKHR(instance, surface, nullptr); //surface must be deleted before the instance
		vkDestroyInstance(instance, nullptr);
		vkDestroyDevice(device, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	GLFWwindow* window;
	VkInstance instance;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device; //logical device opposed to physical device
	
	VkQueue graphicsQueue;
	VkQueue presentQueue;
};

int main(){
	Application app;

	try{
		app.run();
	}catch (const std::exception& e){
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

