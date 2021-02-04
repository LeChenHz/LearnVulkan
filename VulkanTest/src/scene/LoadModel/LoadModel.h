#pragma once
#include "../SceneBase.h"



class LoadModel :public SceneBase {

	const std::string MODEL_PATH = "../VulkanTest/res/model/viking_room.obj";
	const std::string TEXTURE_PATH = "../VulkanTest/res/texture/viking_room.png";

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	/* mipmap */
	uint32_t mipLevels;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	void cleanup();
	void cleanupSwapChain();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createRenderPass();
	void createFramebuffers();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createDepthResources();
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createImageViews();
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void drawFrame();
	void updateUniformBuffer(uint32_t currentImage);

	//model
	void loadModel();
	void createVertexBuffer();
	void createIndexBuffer();

	//mipmap
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	
};