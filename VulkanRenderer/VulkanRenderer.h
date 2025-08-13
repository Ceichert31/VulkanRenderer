#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPSOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>

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

const int MAX_FRAMES_IN_FLIGHT = 2;

//const std::string WORKING_DIRECTORY = "../../../";

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
	std::optional<uint32_t> graphicsFamily{};
	std::optional<uint32_t> presentFamily{};

	bool isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

/// <summary>
/// All details necessary for creating a swapchain
/// </summary>
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats{};
	std::vector<VkPresentModeKHR> presentModes{};
};

/// <summary>
/// Holds all vertex information
/// </summary>
struct Vertex
{
	glm::vec2 position{};
	glm::vec3 color{};

	///Defines the stride of the binding
	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		//Use instance here for instanced rendering
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
};

const std::vector<Vertex> VERTICES = {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

/// <summary>
/// The main vulkan rendering pipeline
/// </summary>
class VulkanRenderer {
public:
	VulkanRenderer();
	~VulkanRenderer();

	void init();
	void cleanup();
	void update();
	void drawFrame();

private:

	void createWindow();
	void createInstance();
	void createSurface();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	void pickPhysicalDevice();
	void createLogicalDevice();

	void createSwapChain();
	void createImageViews();

	void recreateSwapChain();
	void cleanupSwapChain();

	//Pipeline and renderer creation
	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void createSyncObjects();

	int getDeviceSuitablility(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	//Swap chain supporting functions
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
	SwapChainSupportDetails getSwapChainSupport(VkPhysicalDevice device) const;
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector <VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkShaderModule createShaderModule(const std::vector<char>& code) const;

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
	VkInstance mInstance{};
	VkDebugUtilsMessengerEXT mDebugMessenger{};
	GLFWwindow* mpWindow{};
	VkDevice mDevice{};

	VkPhysicalDevice mPhysicalDevice{};
	VkQueue mGraphicsQueue{};
	VkQueue mPresentQueue{};
	VkSurfaceKHR mSurface{};
	VkSwapchainKHR mSwapChain{};
	std::vector<VkImage> mSwapChainImages{};
	std::vector<VkImageView> mSwapChainImageViews{};
	std::vector<VkFramebuffer> mSwapChainFramebuffers{};
	VkFormat mSwapChainImageFormat{};
	VkExtent2D mSwapChainExtent{};

	VkRenderPass mRenderPass{};
	VkPipelineLayout mPipelineLayout{};
	VkPipeline mGraphicsPipeline{};
	VkCommandPool mCommandPool{};
	std::vector<VkCommandBuffer> mCommandBuffers{};

	//Sync objects
	std::vector<VkSemaphore> mImageAvailableSemaphore{};
	std::vector<VkSemaphore> mRenderFinishedSemaphore{};
	std::vector<VkFence> mInFlightFence{};

	bool mFramebufferResized{};

	uint32_t mCurrentFrame{};
};