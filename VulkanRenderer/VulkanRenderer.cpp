#include "VulkanRenderer.h"
#include <vector>

void HelloTriangleApp::run()
{
	initWindow();
	init();
	update();
	cleanup();
}

void HelloTriangleApp::initWindow()
{
	glfwInit();

	//Tell glfw we aren't using opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//Disable window resizing
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mpWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);


}

void HelloTriangleApp::init()
{
	createInstance();
}

void HelloTriangleApp::update()
{
	while (!glfwWindowShouldClose(mpWindow))
	{
		glfwPollEvents();
	}
}

void HelloTriangleApp::cleanup()
{
	//Deallocate in reverse order to avoid dependency issues
	vkDestroyInstance(mInstance, nullptr);

	glfwDestroyWindow(mpWindow);

	glfwTerminate();
}

void HelloTriangleApp::createInstance()
{
	VkApplicationInfo appinfo{};
	appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appinfo.pApplicationName = "Hello Triange";
	appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appinfo.pEngineName = "No Engine";
	appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appinfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appinfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount = 0;

	//Create instace with above settings
	if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create instance!\n");
	}

	//Check we have the vulkan extensions required by GLFW
	if (!hasRequiredExtensions())
	{
		throw std::runtime_error("ERROR: Missing Required Vulkan Extensions");
	}
}

bool HelloTriangleApp::hasRequiredExtensions()
{

	//Retrieve extension count (Can specify layer with first parameter)
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	//Create a vector of properties
	std::vector<VkExtensionProperties> extensions(extensionCount);

	//Retrieve extension data
	vkEnumerateInstanceExtensionProperties(
		nullptr,
		&extensionCount,
		extensions.data()
	);

	uint32_t requiredCount = 0;

	//Get required vulkan extensions
	glfwGetRequiredInstanceExtensions(&requiredCount);

	if (extensionCount < requiredCount)
	{
		std::cout << "Available extensions:\n";
		for (const auto& ext : extensions)
		{
			std::cout << '\t' << ext.extensionName << std::endl << ext.specVersion << std::endl;
		}

		return false;
	}

	return true;
}
