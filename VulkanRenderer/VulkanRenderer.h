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

class HelloTriangleApp {
public:

	void run();

private:

	void initWindow();
	void init();
	void update();
	void cleanup();

	void createInstance();
	VkResult vkCreateInstance_Ext(const VkInstanceCreateInfo* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkInstance* instance);

	bool hasRequiredExtensions();

	bool checkValidationLayerSupport();

	std::vector<const char*> getRequiredExtensions();

	VkInstance mInstance;
	GLFWwindow* mpWindow;
};