#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

//Validation layers
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else // NDEBUG
const bool enabledValidationLayers = true;
#endif

class GraphicsPipeline {
public:
	GraphicsPipeline();
	~GraphicsPipeline();

	void init();
	void update();
	void cleanup();

private:

	void initWindow();
	void createInstance();

	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);

	void setupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	VkResult vkCreateInstance_Ext(const VkInstanceCreateInfo* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkInstance* instance);

	bool hasRequiredExtensions();
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();

	//Debug message callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);

	//Proxy functions
	VkResult createDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void destroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	//Member variables
	VkInstance mInstance;
	VkDebugUtilsMessengerEXT mDebugMessenger;
	GLFWwindow* mpWindow;
};