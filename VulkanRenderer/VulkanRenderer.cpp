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
}
