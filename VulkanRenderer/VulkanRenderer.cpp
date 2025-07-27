#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer() {}

VulkanRenderer::~VulkanRenderer()
{
	cleanup();
}

/// <summary>
/// Initializes all GLFW and Vulkan components
/// </summary>
void VulkanRenderer::init()
{
	createWindow();
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffer();
	createSyncObjects();
}

/// <summary>
/// Called when object is destroyed
/// </summary>
void VulkanRenderer::cleanup()
{
	vkDestroySemaphore(mDevice, mImageAvailableSemaphore, nullptr);
	vkDestroySemaphore(mDevice, mRenderFinishedSemaphore, nullptr);
	vkDestroyFence(mDevice, mInFlightFence, nullptr);

	vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

	for (auto framebuffer : mSwapChainFramebuffers)
	{
		vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
	}

	vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);

	vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
	
	vkDestroyRenderPass(mDevice, mRenderPass, nullptr);

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
	//Main loop
	while (!glfwWindowShouldClose(mpWindow))
	{
		glfwPollEvents();
		drawFrame();
	}

	//Wait for async operations to stop before cleaning up
	vkDeviceWaitIdle(mDevice);
}

/// <summary>
/// Submits commands to the command buffer to draw a frame
/// </summary>
void VulkanRenderer::drawFrame()
{
	//Wait for fence without timeout
	vkWaitForFences(mDevice, 1, &mInFlightFence, VK_TRUE, UINT64_MAX);

	//Reset fence afterwards
	vkResetFences(mDevice, 1, &mInFlightFence);

	//Get the index of the next image we will render
	uint32_t imageIndex;
	vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX,
		mImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	//Reset command buffer before recording
	vkResetCommandBuffer(mCommandBuffer, 0);

	recordCommandBuffer(mCommandBuffer, imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	//Have semaphores wait at color writing stage
	VkSemaphore waitSemaphores[] = { mImageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffer;

	//This semaphore is signaled once command execution is completed
	VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFence) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to submit draw command buffer!\n");
	}

	//Presentation info
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	//Set our signal semaphore as the wait semaphore so 
	//the renderer waits until the commands are executed to render
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { mSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(mPresentQueue, &presentInfo);
}

#pragma region Window/Vulkan Creation Functions
/// <summary>
/// Creates a GLFW window
/// </summary>
void VulkanRenderer::createWindow()
{
	glfwInit();

	//Tell glfw we aren't using opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//Disable window resizing
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mpWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

/// <summary>
/// Creates the vulkan application and instance
/// </summary>
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

/// <summary>
/// Creates the window surface
/// </summary>
void VulkanRenderer::createSurface()
{
	if (glfwCreateWindowSurface(mInstance, mpWindow, nullptr, &mSurface)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create a window surface!\n");
	}
}
#pragma endregion

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
#pragma endregion

#pragma region Swap Chain Functions

/// <summary>
/// Creates the swap chain using the swap chains supporting functions
/// </summary>
void VulkanRenderer::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = getSwapChainSupport(mPhysicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);

	//Cache image format for use in swap chain image creation
	mSwapChainImageFormat = surfaceFormat.format;

	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);

	mSwapChainExtent = chooseSwapExtent(swapChainSupport.capabilities);

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
	createInfo.imageExtent = mSwapChainExtent;
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

/// <summary>
/// Creates image views and adds them to a memeber variable
/// </summary>
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

/// <summary>
/// Returns the swap chain support details of this machine
/// </summary>
/// <param name="device"></param>
/// <returns></returns>
SwapChainSupportDetails VulkanRenderer::getSwapChainSupport(VkPhysicalDevice device) const
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
#pragma endregion

#pragma region Rendering Setup Functions
/// <summary>
/// Creates the render pass structure
/// </summary>
void VulkanRenderer::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = mSwapChainImageFormat;

	//Only 1 bit because we aren't multisampling as of right now
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	//We are clearing the framebuffer each frame 
	// and storing rendered contents to memory
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	//Discarding stencil framebuffer
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//Set image layout to be unspecified
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//Sub pass layout
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Sub pass description
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	//Render create info
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create render pass!\n");
	}
}

/// <summary>
/// Creates the programmable and fixed functions in the graphics pipeline
/// </summary>
void VulkanRenderer::createGraphicsPipeline()
{
	//Cache shader code as binary
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

	//Fixed functions
	//Specify what parts of the pipeline we want to change during runtime
	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	//Set vertex binding to null for now because we are using hard-coded values
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	//Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//Viewport
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)mSwapChainExtent.width;
	viewport.height = (float)mSwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//Viewport scissor
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = mSwapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	//Rasterizer 
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	//If enabled, instead of clipping geometry out of far-plane it is clamped
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	//Change render mode here (wireframe, fill, point)
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	//Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	//Depth and stencil testing
	VkPipelineDepthStencilStateCreateInfo* depthBuffer = nullptr;

	//Color Blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	//Blending operation settings
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	//Framebuffer blend constants
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	//Specify uniforms in pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create pipeline layout!\n");
	}

	//Create the graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; //Disabled for now
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;

	pipelineInfo.layout = mPipelineLayout;
	pipelineInfo.renderPass = mRenderPass;
	pipelineInfo.subpass = 0;

	//Used to set up a new pipeline from an exsisting one
	//See (VK_PIPELINE_CREATE_DERIVATIVE_BIT)
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create Graphics Pipeline!\n");
	}

	//Cleanup shader modules
	vkDestroyShaderModule(mDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);
}

/// <summary>
/// Initializes framebuffers using the swapchains formats and settings
/// </summary>
void VulkanRenderer::createFramebuffers()
{
	//Resize to hold all framebuffers
	mSwapChainFramebuffers.resize(mSwapChainImageViews.size());

	//Create framebuffers from image views
	for (size_t i = 0; i < mSwapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = {
			mSwapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = mSwapChainExtent.width;
		framebufferInfo.height = mSwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("ERROR: Failed to create framebuffer: " + i);
		}
	}
}

/// <summary>
/// Creates a command pool using queue family indices
/// </summary>
void VulkanRenderer::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(mPhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create command pool!\n");
	}
}

/// <summary>
/// Creates the command buffer using the command pool  
/// </summary>
void VulkanRenderer::createCommandBuffer()
{
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = mCommandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(mDevice, &allocateInfo, &mCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create command buffers!\n");
	}
}

/// <summary>
/// Starts recording the command buffer and the render pass
/// </summary>
/// <param name="commandBuffer"></param>
/// <param name="imageIndex"></param>
void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	//Begin recording command buffer
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(mCommandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to begin recording command buffer!\n");
	}

	//Start render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = mRenderPass;
	renderPassInfo.framebuffer = mSwapChainFramebuffers[imageIndex];

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = mSwapChainExtent;

	VkClearValue clearColor = { {{ 0.0f, 0.0f, 0.0f, 1.0f }} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(mCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//Bind the graphics pipeline (VK_PIPELINE_BIND_POINT_COMPUTE for compute pipelines)
	vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

	//Specify our dynamic pipeline values here
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)mSwapChainExtent.width;
	viewport.height = (float)mSwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//Update our viewport
	vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = mSwapChainExtent;

	//Update our scissor
	vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

	//Issue draw command for triangle
	vkCmdDraw(mCommandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(mCommandBuffer);
	if (vkEndCommandBuffer(mCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to record command buffer!\n");
	}
}

/// <summary>
/// Creates Semaphores and fences that are used to keep the CPU and GPU in sync
/// </summary>
void VulkanRenderer::createSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFence) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create semaphores!\n");
	}
}
#pragma endregion

#pragma region Shader/Loading Functions
/// <summary>
/// Takes a binary vector of shader code and creates a VkShaderModule
/// </summary>
/// <param name="code"></param>
/// <returns></returns>
VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) const
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

/// <summary>
/// Tell the debug messenger what kind of messages we would like (errors, warnings, info)
/// </summary>
/// <param name="createInfo"></param>
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

#pragma region Required Extensions/Layers Functions
/// <summary>
/// Checks whether this machine has the required extensions to run the program
/// </summary>
/// <returns></returns>
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

/// <summary>
/// Returns the required vulkan extensions to run this program
/// </summary>
/// <returns></returns>
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

/// <summary>
/// Checks whether the required validation layers are present on this machine
/// </summary>
/// <returns></returns>
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
#pragma endregion

#pragma region Helper Functions
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
#pragma endregion