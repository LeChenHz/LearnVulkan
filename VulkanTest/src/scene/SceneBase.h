#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define GLM_FORCE_RADIANS //ʹ�û�����Ϊ��λ
#define GLM_FORCE_DEPTH_ZERO_TO_ONE //glm��ͶӰ����ʹ��opengl���ֵ��Χ��-1.0��-1.0��,�ú�ʹ�ã�0,1��
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
#include <chrono> //ʹ�ü�ʱ����


//�Ƿ�����У���
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2; //�������Ⱦ֡��

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	/* ����vertex�ṹ��Ķ������ݴ�ŷ�ʽ */
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		/* 
			ÿ��������������ݱ���װ��ͬһ���ṹ������
			binding ָ������������Ϣ�ڰ�������Ϣ�����е����������ڼ�����������Ϣ��
			stride ָ����Ŀ֮���ࣨ�ֽڣ�
			inputRate ָ����Ŀ�����ȣ��У�
			- VK_VERTEX_INPUT_RATE_VERTEX����һ������������Ϊ��Ŀ������
			- VK_VERTEX_INPUT_RATE_INSTANCE����һ��ʵ��������Ϊ��Ŀ������

		*/
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	/*
		VkVertexInputAttributeDescription�ṹ��������������
		- binding ָ������������Դ���������binding��
		- location ָ���ڶ�����ɫ���еĴ��λ�ã�����ɫ����Ӧ
		- format ָ����������,�������У�
			float��VK_FORMAT_R32_SFLOAT
			vec2��VK_FORMAT_R32G32_SFLOAT
			vec3��VK_FORMAT_R32G32B32_SFLOAT
			vec4��VK_FORMAT_R32G32B32A32_SFLOAT
			��ָ����ʽͨ����С����ɫ���еĶ���BGAͨ����ʹ�ã�0,0,1����Ϊ�������ͨ��ֵ��������UINT,SINT�ģ����磺
			ivec2��VK_FORMAT_R32G32_SINT���������������ģ�ÿ������������32λ�������͵�����
			uvec4��VK_FORMAT_R32G32B32A32_UINT�������ĸ������ģ�ÿ������������32λ�޷������͵�����
			double��VK_FORMAT_R64_SFLOAT��һ��˫����(64λ)������
		- offset ָ���˿�ʼ��ȡ�����������ݵ�λ�ã����ڶ������ݰ����˶������ԣ��������Ǵ洢�����鿪ʼλ�ã�
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
	/* ��һ��const */
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
	��Ϊ�������ؽ�������ͣ�����-1��ʾû���ҵ���������Ķ�����
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

	/* ÿһ֡�������ź��� */
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0;

	bool framebufferResized = false;//���ڴ�С�Ƿ�ı�

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

	/* ��������ʹ��,֮ǰ�汾ֻ��2��������aspectFlags����VK_IMAGE_ASPECT_COLOR_BIT��������Ҫ���޸�Ϊһ���������� */
	virtual VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	virtual void createImageViews();

	virtual void createRenderPass();

	virtual void createGraphicsPipeline();

	virtual void createFramebuffers();
	
	virtual void createCommandPool();

	virtual void createVertexBuffer();

	virtual void createIndexBuffer();

	/* ����createVertexBuffer�У����ڸ����������� */
	virtual void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	/* 
		�ڴ洫��ָ�ͨ��Ϊʹ�øô���ָ���ָ��崴���µ�commandPool����
		����Ϊ�ڴ洫��ָ���������ڶ̣�ʹ�ö����ĸ����Ż��� 
		�������µ�commandpool��Ҫָ��VK_COMMAND_POOL_CREATE_TRANSIENT_BIT��ǣ�����û�����µ�commandpool��
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
