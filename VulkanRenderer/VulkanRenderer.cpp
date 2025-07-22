#include "VulkanRenderer.h"

void VulkanRenderer::initWindow()
{
	glfwInit();

	//Tell glfw we aren't using opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//Disable window resizing
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mpWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

VulkanRenderer::VulkanRenderer()
{
	mInstance = VkInstance();
	mDebugMessenger = VkDebugUtilsMessengerEXT();
	mpWindow = nullptr;
}

VulkanRenderer::~VulkanRenderer()
{
	cleanup();
}

void VulkanRenderer::init()
{
	initWindow();
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createGraphicsPipeline();
}

void VulkanRenderer::cleanup()
{
	for (auto imageView : mSwapChainImageViews)
	{
		vkDestroyImageView(mDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);

	vkDestroyDevice(mDevice, nullptr);

	glfwDestroyWindow(mpWindow);

	if (enabledValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(mInstance,
			mDebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);

	//Destroy instance last to prevent memory leaks
	vkDestroyInstance(mInstance, nullptr);

	glfwTerminate();
}

void VulkanRenderer::update()
{
	while (!glfwWindowShouldClose(mpWindow))
	{
		glfwPollEvents();
	}
}

void VulkanRenderer::createInstance()
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

void VulkanRenderer::createSurface()
{
	if (glfwCreateWindowSurface(mInstance, mpWindow, nullptr, &mSurface)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create a window surface!\n");
	}
}

#pragma region Device Creation Methods
/// <summary>
/// Sets up the GPU and selects the best available
/// </summary>
void VulkanRenderer::pickPhysicalDevice()
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
int VulkanRenderer::getDeviceSuitablility(VkPhysicalDevice device)
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
	if (!indices.isComplete() || !checkDeviceExtensionSupport(device))
	{
		return 0;
	}

	//Check for swap chain support
	SwapChainSupportDetails swapChainSupport = getSwapChainSupport(device);
	if (swapChainSupport.formats.empty() && swapChainSupport.presentModes.empty())
	{
		return 0;
	}
	else
	{
		suitability += swapChainSupport.formats.size() + swapChainSupport.presentModes.size();
	}

	return suitability;
}
/// <summary>
/// Checks whether a device can support the swap chain and associated required extensions
/// </summary>
/// <param name="device"></param>
/// <returns></returns>
bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	//Enumerate extension and check if required extensions are all present

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);

	//Retrieve extension data
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	//Remove required extensions from the set
	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	//If the set is empty it means we have all our required 
	return requiredExtensions.empty();
}
/// <summary>
/// Creates a logical device using previously created physical device
/// </summary>
void VulkanRenderer::createLogicalDevice()
{
	//Queue family
	QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);

	//Unique list of queue families
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfoList;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	//Required even if only using one queue
	float queuePriority = 1.0f;

	//Setup each queue family and add it to the list
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfoList.push_back(queueCreateInfo);
	}

	//Define what features we want
	VkPhysicalDeviceFeatures deviceFeatures{};

	//Device creation info
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	//Queue info
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfoList.data();
	deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfoList.size();

	//Set features
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	//Extension settings
	deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	//Validation layer settings
	if (enabledValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	//Create device
	if (vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create logical device!\n");
	}

	//Use index 0 since we are using only one queue
	vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);

	vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mPresentQueue);
}
#pragma endregion
#pragma region Swap Chain Methods
SwapChainSupportDetails VulkanRenderer::getSwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &details.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);
	
	//Resize vector to hold surface format data
	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, details.formats.data());
	}



	return details;
}

/// <summary>
/// Chooses the best available surface format
/// </summary>
/// <param name="availableFormats"></param>
/// <returns></returns>
VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	//Return first format if none meet our specifications
	return availableFormats[0];
}

/// <summary>
/// Chooses the best available present mode
/// </summary>
/// <param name="availablePresentModes"></param>
/// <returns></returns>
VkPresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

/// <summary>
/// Chooses the min and maximum size of the display
/// </summary>
/// <param name="capabilities"></param>
/// <returns></returns>
VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(mpWindow, &width, &height);

		VkExtent2D actualExtent = {
			(uint32_t)width,
			(uint32_t)height
		};

		//Clamp width and height
		actualExtent.width = std::clamp(actualExtent.width, 
			capabilities.minImageExtent.width, 
			capabilities.maxImageExtent.width);

		actualExtent.height = std::clamp(actualExtent.height,
			capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void VulkanRenderer::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = getSwapChainSupport(mPhysicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);

	//Cache image format for use in swap chain image creation
	mSwapChainImageFormat = surfaceFormat.format;

	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);

	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	//Minimum number of images to function
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && 
		imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mSurface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = mSwapChainImageFormat;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create swap chain!\n");
	}

	//Retrieve max image count size and resize vector
	vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr);
	mSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mSwapChainImages.data());

}

void VulkanRenderer::createImageViews()
{
	mSwapChainImageViews.resize(mSwapChainImages.size());

	for (size_t i = 0; i < mSwapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = mSwapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = mSwapChainImageFormat;

		//Mapping colors
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapChainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("ERROR: Failed to create image views");
		}
	}
}
#pragma endregion

void VulkanRenderer::createGraphicsPipeline()
{
	auto vertShaderCode = readFile(WORKING_DIRECTORY + "shaders/vert.spv");
	auto fragShaderCode = readFile(WORKING_DIRECTORY + "shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	//Programmable pipeline creation info for vertex shader
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	//We can set shader constants here
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	//Creation info for fragment shader
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//Cleanup shader modules
	vkDestroyShaderModule(mDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);
}

/// <summary>
/// Takes a binary vector of shader code and creates a VkShaderModule
/// </summary>
/// <param name="code"></param>
/// <returns></returns>
VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create shader module!\n");
	}
	return shaderModule;
}

/// <summary>
/// Returns queue families that support specfic requirements
/// </summary>
/// <param name="device"></param>
/// <returns></returns>
QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice device) const
{
	QueueFamilyIndices indices;

	//Get queue family count
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	//Get data using family count and store in vector
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device,
		&queueFamilyCount, queueFamilies.data());

	//NOTE: Refactor to be encapsulated in one loop
	//Find a queue family that supports VK_QUEUE_GRAPHICS_BIT
	int queueFamilyIndex = 0;
	VkBool32 presentSupport = false;
	
	//Find available queue families that support graphics commands 
	// and windows surface presentation
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = queueFamilyIndex;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, mSurface, &presentSupport);
		}

		//Store presentation family queue index if presentation is supported
		if (presentSupport)
		{
			indices.presentFamily = queueFamilyIndex;
		}

		//Break out of loop early if complete
		if (indices.isComplete())
		{
			break;
		}
		queueFamilyIndex++;
	}

	return indices;
}

/// <summary>
/// Wrapper for vkCreateInstance that handles and prevents errors
/// </summary>
/// <param name="pCreateInfo"></param>
/// <param name="pAllocator"></param>
/// <param name="pInstance"></param>
/// <returns></returns>
VkResult VulkanRenderer::vkCreateInstance_Ext(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
	if (pCreateInfo == nullptr || pInstance == nullptr)
	{
		std::cout << "Null pointer passed to required parameter!\n";
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	return vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

bool VulkanRenderer::hasRequiredExtensions()
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

std::vector<const char*> VulkanRenderer::getRequiredExtensions()
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

bool VulkanRenderer::checkValidationLayerSupport()
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
#pragma region Debug Messenger Methods
/// <summary>
/// Initializes the debug messenger and flags which messeges to recieve
/// </summary>
void VulkanRenderer::setupDebugMessenger()
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

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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
/// Callback function for vulkan debug messages
/// </summary>
/// <param name="messageSeverity">The severity of the message 
/// (Diagnostic, Info, Warning, Error) </param>
/// <param name="messageType">What the message is relating to
/// (General, Validation, Performance)</param>
/// <param name="pCallbackData">Contains the details of the message</param>
/// <param name="pUserData"></param>
/// <returns></returns>
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
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
VkResult VulkanRenderer::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
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
void VulkanRenderer::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
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

#pragma endregion

#pragma region Helper Methods
/// <summary>
/// Takes a filename and returns a binary vector
/// </summary>
/// <param name="filename"></param>
/// <returns></returns>
std::vector<char> readFile(const std::string& filename)
{
	//Start reading file from the end to gauge file size
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("ERROR: Failed to open file named: " + filename);
	}

	//Get file size
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	//Start reading file from start
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}
#pragma endregion