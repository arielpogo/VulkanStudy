#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

#include <cstring>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifndef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

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
		for(const auto& e : extensions){
			std::cout << '\t' << e.extensionName << '\n';
		}
			
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

		//poll for all validation layers
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		bool found = false;
		for(const char* layerName : validationLayers){
			for(const auto& availableLayer : availableLayers){
				if(strcmp(layerName, availableLayer.layerName) == 0){
					found = true;
					break;
				}
				if(!found) return false;
			}
		}

		return false;
	}

	void mainLoop(){
		while(!glfwWindowShouldClose(window)){
			glfwPollEvents();
		}

	}

	void cleanup(){
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();

	}

	GLFWwindow* window;
	VkInstance instance;
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

