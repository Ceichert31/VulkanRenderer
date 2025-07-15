#include "VulkanRenderer.h"

void GraphicsPipeline::initWindow()
{
	glfwInit();

	//Tell glfw we aren't using opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//Disable window resizing
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mpWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

GraphicsPipeline::GraphicsPipeline()
{
	mInstance = VkInstance();
	mDebugMessenger = VkDebugUtilsMessengerEXT();
	mpWindow = nullptr;
}

GraphicsPipeline::~GraphicsPipeline()
{
	cleanup();
}

void GraphicsPipeline::init()
{
	initWindow();
	createInstance();
	setupDebugMessenger();
	pickPhysicalDevice();
	createLogicalDevice();
}

void GraphicsPipeline::cleanup()
{
	//Clean up physical device
	vkDestroyDevice(mDevice, nullptr);

	//Deallocate in reverse order to avoid dependency issues
	glfwDestroyWindow(mpWindow);

	if (enabledValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(mInstance,
			mDebugMessenger, nullptr);
	}

	//Destroy instance last to prevent memory leaks
	vkDestroyInstance(mInstance, nullptr);

	glfwTerminate();
}

void GraphicsPipeline::update()
{
	while (!glfwWindowShouldClose(mpWindow))
	{
		glfwPollEvents();
	}
}

void GraphicsPipeline::createInstance()
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

	//Create a second debug messenger for debugging createInstance
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

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

		//Setup debug messenger now so we can debug our instance creation
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)
			&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	//Create instace with VkInstanceCreateInfo
	if (vkCreateInstance_Ext(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create instance!\n");
	}
}

/// <summary>
/// Sets up the GPU and selects the best available
/// </summary>
void GraphicsPipeline::pickPhysicalDevice()
{
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	//Query number of devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);

	//Check device count
	if (deviceCount < 1)
	{
		throw std::runtime_error("ERROR: Failed to find GPUs with Vulkan Support!\n");
	}

	//Allocate array to hold all handles
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

	//Map of available devices 
	std::multimap<int, VkPhysicalDevice> availableDevices;

	//Get suitability of all devices and add to ordered map
	for (const auto& device : devices)
	{
		int suitability = getDeviceSuitablility(device);
		availableDevices.insert(std::make_pair(suitability, device));
	}

	//Get highest rated device, if there is no device throw error
	if (availableDevices.rbegin()->first > 0)
	{
		physicalDevice = availableDevices.rbegin()->second;
	}
	else
	{
		throw std::runtime_error("ERROR: Failed to find a suitable GPU\n");
	}

	mPhysicalDevice = physicalDevice;
}

/// <summary>
/// Grades devices based on device features and properties
/// </summary>
/// <param name="device"></param>
/// <returns></returns>
int GraphicsPipeline::getDeviceSuitablility(VkPhysicalDevice device)
{
	int suitability = 0;

	//Get device properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	//Get device features
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	//Discrete GPUs have a performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		suitability += 1000;
	}

	//Max possible size of textures affects graphics quality
	suitability += deviceProperties.limits.maxImageDimension2D;

	//Program can't function without geometry shader
	if (!deviceFeatures.geometryShader)
	{
		return 0;
	}

	//Check if indices contains any queue families
	QueueFamilyIndices indices = findQueueFamilies(device);
	if (!indices.isComplete())
	{
		return 0;
	}

	return suitability;
}

/// <summary>
/// Creates a logical device using previously created physical device
/// </summary>
void GraphicsPipeline::createLogicalDevice()
{
	//Queue family
	QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);

	//Queue creation info
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;

	//Required even if only using one queue
	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	//Define what features we want
	VkPhysicalDeviceFeatures deviceFeatures{};

	//Device creation info
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	//Queue info
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;

	//Set features
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = 0;

	if (enabledValidationLayers)
	{
		createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	//Create device
	if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create logical device!\n");
	}

	//Use index 0 since we are using only one queue
	vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
}

QueueFamilyIndices GraphicsPipeline::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	//Get queue family count
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	//Get data using family count and store in vector
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device,
		&queueFamilyCount, queueFamilies.data());

	//Find a queue family that supports VK_QUEUE_GRAPHICS_BIT
	int supportedFamilies = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = supportedFamilies;
		}

		if (indices.isComplete())
		{
			break;
		}
		supportedFamilies++;
	}

	return indices;
}

/// <summary>
/// Initializes the debug messenger and flags which messeges to recieve
/// </summary>
void GraphicsPipeline::setupDebugMessenger()
{
	if (!enabledValidationLayers) return;

	//Populate info for messenger
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(createInfo);

	//Confirm messenger is setup
	if (createDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to setup debug messenger!\n");
	}
}

void GraphicsPipeline::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType =
		VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	//Call callback for these severities 
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	//Call callback for these message types
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

	//Set callback
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

/// <summary>
/// Wrapper for vkCreateInstance that handles and prevents errors
/// </summary>
/// <param name="pCreateInfo"></param>
/// <param name="pAllocator"></param>
/// <param name="pInstance"></param>
/// <returns></returns>
VkResult GraphicsPipeline::vkCreateInstance_Ext(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
	if (pCreateInfo == nullptr || pInstance == nullptr)
	{
		std::cout << "Null pointer passed to required parameter!\n";
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	return vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

bool GraphicsPipeline::hasRequiredExtensions()
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

std::vector<const char*> GraphicsPipeline::getRequiredExtensions()
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

bool GraphicsPipeline::checkValidationLayerSupport()
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

/// <summary>
/// Callback function for vulkan debug messages
/// </summary>
/// <param name="messageSeverity">The severity of the message 
/// (Diagnostic, Info, Warning, Error) </param>
/// <param name="messageType">What the message is relating to
/// (General, Validation, Performance)</param>
/// <param name="pCallbackData">Contains the details of the message</param>
/// <param name="pUserData"></param>
/// <returns></returns>
VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsPipeline::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	
	//Can optionally add debug for pObjects to be printed when severity is an error
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		//Debug attached objects
	}

	return VK_FALSE;
}

/// <summary>
/// Looks up the vkCreateDebugUtilsMessengerEXT 
/// function and passes our debug messenger to it
/// </summary>
/// <param name="instance"></param>
/// <param name="pCreateInfo"></param>
/// <param name="pAllocator"></param>
/// <param name="pDebugMessenger"></param>
/// <returns></returns>
VkResult GraphicsPipeline::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	//Try to load messenger function
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkCreateDebugUtilsMessengerEXT");

	//Run function if not null
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/// <summary>
/// Looks up the vkDestroyDebugUtilsMessengerEXT 
/// function and passes our debug messenger to it
/// </summary>
/// <param name="instance"></param>
/// <param name="debugMessenger"></param>
/// <param name="pAllocator"></param>
void GraphicsPipeline::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	//Try to load deletion function from address
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkDestroyDebugUtilsMessengerEXT");

	//Run function if not null
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}
