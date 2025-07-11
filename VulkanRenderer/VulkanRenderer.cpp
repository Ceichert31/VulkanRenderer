#include "VulkanRenderer.h"

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
	//Setup app info
	VkApplicationInfo appinfo{};
	appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appinfo.pApplicationName = "Hello Triange";
	appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appinfo.pEngineName = "No Engine";
	appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appinfo.apiVersion = VK_API_VERSION_1_0;

	//Setup instance creation info
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appinfo;

	//Get extensions
	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	//Check we have the vulkan extensions required by GLFW
	if (!hasRequiredExtensions())
	{
		throw std::runtime_error("ERROR: Missing Required Vulkan Extensions\n");
	}

	//Check we can use validation layers
	if (enabledValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("ERROR: Missing Required Validation Layers!\n");
	}

	//Fill info in VK create info
	if (enabledValidationLayers)
	{
		createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	//Create instace with VkInstanceCreateInfo
	if (vkCreateInstance_Ext(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create instance!\n");
	}
}

VkResult HelloTriangleApp::vkCreateInstance_Ext(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
	if (pCreateInfo == nullptr || pInstance == nullptr)
	{
		std::cout << "Null pointer passed to required parameter!\n";
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	return vkCreateInstance(pCreateInfo, pAllocator, pInstance);
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

bool HelloTriangleApp::checkValidationLayerSupport()
{
	//Get Layer count
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	//Retrieve data on available layers
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	//Check that all validation layers exsist in available layers
	for (const char* requiredLayer : validationLayers)
	{
		for (const auto& availableLayer : availableLayers)
		{
			if (strcmp(requiredLayer, availableLayer.layerName) == 0)
				return true;
		}
	}

	return false;
}

std::vector<const char*> HelloTriangleApp::getRequiredExtensions()
{
	//Retrieve extension count
	uint32_t extensionCount = 0;
	const char** glfwExtensions;

	//Get required vulkan extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	std::vector<const char*> extensions(glfwExtensions, 
		glfwExtensions + extensionCount);

	//Add debug extension
	if (enabledValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}
