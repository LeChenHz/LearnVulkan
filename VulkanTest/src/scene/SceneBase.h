#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define GLM_FORCE_RADIANS //使用弧度作为单位
#define GLM_FORCE_DEPTH_ZERO_TO_ONE //glm库投影矩阵使用opengl深度值范围（-1.0，-1.0）,该宏使用（0,1）
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <cstdlib>
#include <set>
#include <array>
#include <chrono> //使用计时函数


//是否启用校验层
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2; //最大并行渲染帧数

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	/* 返回vertex结构体的顶点数据存放方式 */
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		/* 
			每个顶点的所有数据被包装到同一个结构体数组
			binding 指定保定描述信息在绑定描述信息数组中的索引（即第几个绑定描述信息）
			stride 指定条目之间跨距（字节）
			inputRate 指定条目的粒度，有：
			- VK_VERTEX_INPUT_RATE_VERTEX：以一个顶点数据作为条目的粒度
			- VK_VERTEX_INPUT_RATE_INSTANCE：以一个实例数据作为条目的粒度

		*/
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	/*
		VkVertexInputAttributeDescription结构体描述顶点属性
		- binding 指定顶点数据来源（从上面的binding）
		- location 指定在顶点着色器中的存放位置，和着色器对应
		- format 指定数据类型,常见的有：
			float：VK_FORMAT_R32_SFLOAT
			vec2：VK_FORMAT_R32G32_SFLOAT
			vec3：VK_FORMAT_R32G32B32_SFLOAT
			vec4：VK_FORMAT_R32G32B32A32_SFLOAT
			若指定格式通道数小于着色器中的定义BGA通道会使用（0,0,1）作为多出来的通道值，还有用UINT,SINT的，比如：
			ivec2：VK_FORMAT_R32G32_SINT，具有两个分量的，每个分量类型是32位符号整型的向量
			uvec4：VK_FORMAT_R32G32B32A32_UINT，具有四个分量的，每个分量类型是32位无符号整型的向量
			double：VK_FORMAT_R64_SFLOAT，一个双精度(64位)浮点数
		- offset 指定了开始读取顶点属性数据的位置（由于顶点数据包含了多钟属性，并不都是存储在数组开始位置）
	*/
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
	/* 后一个const */
	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos)
				^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1)
				^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};



/*
	作为函数返回结果的类型，索引-1表示没有找到满足需求的队列族
*/
struct QueueFamilyIndices {
	uint32_t graphicsFamily = -1;//use for logicdevice
	uint32_t presentFamily = -1;// use for window surface

	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >=0;
	}
};

struct SwapChainSupportDetails{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class SceneBase {
public:

	const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0,1,2,2,3,0,
		4,5,6,6,7,4
	};

	const int WIDTH = 800;
	const int HEIGHT = 600;
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device; //logic device

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkCommandBuffer> commandBuffers;

	/* 每一帧独立的信号量 */
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0;

	bool framebufferResized = false;//窗口大小是否改变

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	virtual void run();

	virtual void initWindow();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<SceneBase*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}
	virtual void initVulkan();

	virtual void createDescriptorSetLayout() {}
	virtual void createUniformBuffers() {}
	virtual void createDescriptorPool() {}
	virtual void createDescriptorSets() {}
	virtual void createTextureImage() {}
	virtual void createTextureImageView() {}
	virtual void createTextureSampler() {}
	virtual void createDepthResources() {}
	virtual void loadModel() {}

	virtual void mainLoop();
	virtual void cleanupSwapChain();
 
	virtual std::vector<char> readFile(const std::string& filename);

	virtual VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	virtual void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	virtual void cleanup();

	virtual void recreateSwapChain();

	virtual void createInstance();

	virtual void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	virtual void setupDebugMessenger();

	virtual void createSurface();

	virtual void pickPhysicalDevice();

	virtual void createLogicalDevice();

	virtual void createSwapChain();

	/* 方便纹理使用,之前版本只有2个参数，aspectFlags总是VK_IMAGE_ASPECT_COLOR_BIT，不符合要求，修改为一个函数参数 */
	virtual VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	virtual void createImageViews();

	virtual void createRenderPass();

	virtual void createGraphicsPipeline();

	virtual void createFramebuffers();
	
	virtual void createCommandPool();

	virtual void createVertexBuffer();

	virtual void createIndexBuffer();

	/* 用于createVertexBuffer中，用于辅助创建缓冲 */
	virtual void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	/* 
		内存传输指令。通常为使用该传输指令的指令缓冲创建新的commandPool对象
		（因为内存传输指令生命周期短，使用独立的更好优化） 
		若创建新的commandpool需要指定VK_COMMAND_POOL_CREATE_TRANSIENT_BIT标记（这里没有用新的commandpool）
	*/
	virtual void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	virtual uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	virtual void createCommandBuffers();

	virtual void createSyncObjects();

	virtual void drawFrame();

	virtual VkShaderModule createShaderModule(const std::vector<char>& code);
	virtual VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	virtual VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	virtual VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	virtual SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	virtual bool isDeviceSuitable(VkPhysicalDevice device);

	virtual VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
		VkFormatFeatureFlags features);

	virtual bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	virtual QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	virtual std::vector<const char*>getRequiredExtensions();

	virtual bool checkValidationLayerSupport();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};
