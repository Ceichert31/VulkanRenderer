#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApp {
public:

	void run();

private:

	void initWindow();
	void init();
	void update();
	void cleanup();

	void createInstance();

	VkInstance mInstance;
	GLFWwindow* mpWindow;
};