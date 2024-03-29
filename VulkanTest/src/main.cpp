#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "scene/scene.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>


int main()
{
	//glfwInit();

	//glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//GLFWwindow* window = glfwCreateWindow(800, 600, "LearnVulkan", nullptr, nullptr);
	//
	//uint32_t extensionCount = 0;
	//vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	//
	//std::cout << extensionCount << "extensions supported" << std::endl;
	//
	//glm::mat4 matrix;
	//glm::vec4 vec;
	//auto test = matrix * vec;
	//
	//while (!glfwWindowShouldClose(window)) {
	//	glfwPollEvents();
	//}
	//
	//glfwDestroyWindow(window);

	
	//SceneBase* app = new SceneBase();
	//LoadModel* app = new LoadModel
	CompressedTexture* app = new CompressedTexture();
	//UniformBuffers* app = new UniformBuffers();
	try{
		app->run();
		}catch(const std::exception& e){
			std::cerr << e.what() << std::endl;
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
	

	return 0;
}