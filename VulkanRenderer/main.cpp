#include "VulkanRenderer.h"

#include <stdexcept>
#include <cstdlib>
#include <iostream>

int main() 
{
	GraphicsPipeline app;

	try 
	{
		app.init();
		app.update();
		app.cleanup();
	}
	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}