#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPSOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <optional>

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

/// <summary>
/// A struct that holds information on queue families
/// </summary>
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

/// <summary>
/// The main vulkan rendering pipeline
/// </summary>
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
	void createSurface();

	void pickPhysicalDevice();
	int getDeviceSuitablility(VkPhysicalDevice device);

	void createLogicalDevice();

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

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
	VkDevice mDevice;
	VkPhysicalDevice mPhysicalDevice;
	VkQueue mGraphicsQueue;
	VkQueue mPresentQueue;
	VkSurfaceKHR mSurface;
};