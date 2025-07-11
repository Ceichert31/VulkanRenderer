#include "VulkanRenderer.h"

#include <stdexcept>
#include <cstdlib>
#include <iostream>

int main() 
{
	GraphicsPipeline* graphicsPipeline = new GraphicsPipeline();

	try 
	{
		//Init the graphics pipeline
		graphicsPipeline->init();

		//Update it
		graphicsPipeline->update();

		//Clean it up
		delete graphicsPipeline;
		graphicsPipeline = nullptr;
	}
	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}