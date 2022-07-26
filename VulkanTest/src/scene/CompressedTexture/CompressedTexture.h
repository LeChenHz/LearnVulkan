#pragma once
#include "../SceneBase.h"
#include "ASTCTable.h"
#include "SpvFile.h"


class CompressedTexture :public SceneBase {

	const std::string MODEL_PATH = "../VulkanTest/res/model/viking_room.obj";
	const std::string TEXTURE_PATH = "../VulkanTest/res/texture/viking_room.png";
	const std::string ASTC_TEXTURE_PATH = "../VulkanTest/res/texture/1.data";
	const std::string ETC_TEXTURE_PATH = "../VulkanTest/res/texture/mrfz.etc";
	const std::string GRASS = "../VulkanTest/res/texture/grass.png";

	//msaa
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout decompDescriptorSetLayout;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkBuffer compBuffer;
	VkDeviceMemory compBufferMemory;
	VkPipelineLayout decompPipelineLayout;
	VkPipeline decompPipeline;

	VkDescriptorPool descriptorPool;
	VkDescriptorPool decompDescriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSet> decompDescriptorSets;

	/* mipmap */
	uint32_t mipLevels;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage computeTextureImage;
	VkDeviceMemory computeTextureImageMemory;
	VkImageView computeTextureImageView;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	/* astc image */
	VkImage astcImage;
	VkDeviceMemory astcImageMemory;
	VkImageView astcImageView;

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
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageType imageType = VK_IMAGE_TYPE_2D);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D);
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

	//msaa
	void createColorResources();

	//For compressed texture
	void GenerateCompressedTextureImage();
	void GenerateCompressedTextureImageASTC();
	void GenerateCompressedTextureImageASTC_old();

	shader_spv::SpvData loadDecompressionShader(const char* formatName);

};