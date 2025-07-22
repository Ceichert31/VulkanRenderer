#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPSOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <filesystem>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string WORKING_DIRECTORY = "../../../";

//Validation layers
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const bool enabledValidationLayers = true;

static std::vector<char> readFile(const std::string& filename);

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

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

/// <summary>
/// The main vulkan rendering pipeline
/// </summary>
class VulkanRenderer {
public:
	VulkanRenderer();
	~VulkanRenderer();

	void init();
	void update();
	void cleanup();

private:

	void initWindow();
	void createInstance();
	void createSurface();

	void pickPhysicalDevice();
	void createLogicalDevice();

	void createSwapChain();
	void createImageViews();

	void createGraphicsPipeline();

	int getDeviceSuitablility(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

	SwapChainSupportDetails getSwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector <VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkShaderModule createShaderModule(const std::vector<char>& code);

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

	void setupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

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
	VkSwapchainKHR mSwapChain;
	std::vector<VkImage> mSwapChainImages;
	std::vector<VkImageView> mSwapChainImageViews;
	VkFormat mSwapChainImageFormat;
};