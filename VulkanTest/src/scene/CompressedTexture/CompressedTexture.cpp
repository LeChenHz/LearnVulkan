#include "CompressedTexture.h"
#include <unordered_map>
#define TINYOBJLOADER_IMPLEMENTATION
#include <../common/tiny_obj_loader.h>

void CompressedTexture::cleanup() {
	cleanupSwapChain();

	vkDestroySampler(device, textureSampler, nullptr);
	vkDestroyImageView(device, textureImageView, nullptr);//清除纹理图像视图

	vkDestroyImage(device, textureImage, nullptr);
	vkFreeMemory(device, textureImageMemory, nullptr);

	vkDestroyImage(device, astcImage, nullptr);
	vkFreeMemory(device, astcImageMemory, nullptr);

	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, decompDescriptorSetLayout, nullptr);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(device, commandPool, nullptr);

	vkDestroyDevice(device, nullptr);

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();
}

void CompressedTexture::cleanupSwapChain() {
	vkDestroyImageView(device, colorImageView, nullptr);
	vkDestroyImage(device, colorImage, nullptr);
	vkFreeMemory(device, colorImageMemory, nullptr);

	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);

	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);

	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);

	/* 清理uniform缓冲对象 */
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		vkDestroyBuffer(device, uniformBuffers[i], nullptr);
		vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void CompressedTexture::createDescriptorSetLayout() {
	/*
		- binding 指定着色器使用的描述符绑定索引
		- descriptorType 指定描述符类型
		- descriptorCount 指定数组中元素的个数（可以用数组来指定骨骼动画使用的所有变换矩阵），只有一个Unifrom对象，为1
		- stageFlags 指定哪个着色器阶段使用VK_SHADER_STAGE_ALL_GRAPHICS为全阶段
		- pImmutableSamplers 用于和图像采样相关描述符，暂时设为默认值
	*/
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	/* 组合图像采样器描述符 */
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
		&descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void CompressedTexture::createGraphicsPipeline() {
	auto vertShaderCode = readFile("../VulkanTest/shaders/DepthShader/DepthShadervert.spv");
	auto fragShaderCode = readFile("../VulkanTest/shaders/DepthShader/DepthShaderfrag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	//rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	/* Y轴翻转导致顺时针绘制构成背面。。背面被剔除 */
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaSamples;

	/* 开启深度测试 */
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;//指定深度测试使用的运算，这里小于运算，新片段只有小于深度缓冲中的值才会写入颜色附着
	depthStencil.depthBoundsTestEnable = VK_FALSE;//指定可选的深度测试范围
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

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

	/* 管线布局，使用uniform变量 */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	/* 第二个参数引用一个可选的VkPipelineCache对象 */
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphicspiple!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void CompressedTexture::createRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//多重采样缓冲不能直接用于呈现操作

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;//不需要从深度缓冲中复制数据，因此dont care
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//不需要读取之前深度图像数据，因此undefined
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	/* 添加一个新的颜色附着用于转换多重采样缓冲数据 */
	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = swapChainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	/* 引用刚刚创建的颜色附着 */
	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;//msaa

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void CompressedTexture::createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::array<VkImageView, 3> attachments = {
			colorImageView,
			depthImageView,//加入深度附件
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void CompressedTexture::createUniformBuffers() {
	VkDeviceSize buffersize = sizeof(UniformBufferObject);

	uniformBuffers.resize(swapChainImages.size());
	uniformBuffersMemory.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		createBuffer(buffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers[i], uniformBuffersMemory[i]);
	}
}

void CompressedTexture::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSize = {};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());//每一帧分配一个描述符
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //给纹理描述符用
	poolSize[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	poolInfo.pPoolSizes = poolSize.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());//分配最大描述符集个数

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr,
		&descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void CompressedTexture::createDescriptorSets() {
	/* 创建pool填充 VkDescriptorSetAllocateInfo 结构体 */
	std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	/* 创建描述符集对象。为每一个交换链图像使用相同的描述符布局创建对应的描述符集。但由于描述符布局对象个数要匹配描述符集对象个数
	所以，需要使用多个相同的描述符布局对象 */
	descriptorSets.resize(swapChainImages.size());
	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	/* 通过VkDescriptorBufferInfo结构体来配置描述符引用的缓冲对象 */
	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		/* 指定缓冲对象、偏移、访问的数据范围，range若要访问整个缓冲可以设置VK_WHOLE_SIZE */
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		/* 指定纹理对象描述符 */
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		/*
			更新描述符配置可用vkUpdateDescriptorSets, 以VkWriteDescriptorSet作为参数
			- dstSet 用于指定要更新的描述符集对象
			- dstBinding 指定要更新的缓冲绑定（这里缓冲帮到到0，对应着色器binding = 0）
			- dstArrayElement 描述符可以是数组，所以还需要指定数组第一个元素的索引，这里为0即可
			- descriptorType 和 descriptorCount 和之前创建descriptorlayout一样的参数
			可以通过设置dstArrayElement和descriptorCount成员变量一次更新多个描述符。
			- pBufferInfo 指定描述符引用的缓冲数据
			- pImageInfo 指定描述符引用的图像数据
			- pTexelBufferView 指定描述符引用的缓冲视图（buffer view）暂时不明白用处
		*/
		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		/* vkUpdateDescriptorSets函数可以接受两个数组作为参数：
		VkWriteDescriptorSet结构体数组和VkCopyDescriptorSet结构体数组。后者被用来复制描述符对象 */
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void CompressedTexture::createCommandBuffers() {
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		/* 多个清除值 */
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);//最后的参数必须和索引的数组类型一致
		/*
			使用描述符集，由于不是图形管线独有，计算管线也有因此需要指定
			第4,5,6三个参数分别指描述符集第一个元素索引、需要绑定的描述符集个数、以及用于绑定的描述符集数组
			最后两个参数用于指定动态描述符的数组偏移（后面章节介绍）
		*/
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

}

void CompressedTexture::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();

	/* VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT适合显卡读取的类型 */
	createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);

	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	/*
		不需要该变换结果一样，可能是createRenderPass里面这两句：
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		最终渲染流程结束后会变成后者状态
	*/

	//transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
	//	VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

/* 寻找支持深度图图像数据格式的设备 */
VkFormat CompressedTexture::findDepthFormat() {
	/* 使用的是VK_FORMAT_FEATURE_ 类的标记而不是VK_IMAGE_USAGE_ 类,因为这一类格式包含深度通道（有些还有模板通道） */
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

/* 是否含有模板缓冲通道 */
bool CompressedTexture::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void CompressedTexture::createTextureImage() {
	//int texWidth, texHeight, texChannels;
	//stbi_uc* pixels = stbi_load(GRASS.c_str(), &texWidth, &texHeight, &texChannels,
	//	STBI_rgb_alpha);
	//VkDeviceSize imageSize = texWidth * texHeight * 4;
	//mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	//
	//if (!pixels) {
	//	throw std::runtime_error("failed to load texture image!");
	//}
	//
	///* 创建暂存缓冲 */
	//VkBuffer stagingBuffer;
	//VkDeviceMemory stagingBufferMemory;
	//createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	//	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	//
	//void* data;
	//vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	//memcpy(data, pixels, static_cast<size_t>(imageSize));
	//vkUnmapMemory(device, stagingBufferMemory);
	//
	//stbi_image_free(pixels);
	//
	///*
	//	创建纹理对象,mipmap时加入VK_IMAGE_USAGE_TRANSFER_SRC_BIT 的UsageFlag
	//	因为我们要说那个vkCmdBlitImage指令，该指令执行传输操作，对应的图像纹理需要有数据来源和接受目的的flags
	//	并且该指令对于要处理的图像布局有要求，可以转化到VK_IMAGE_LAYOUT_GENERAL布局，该布局适合大部分指令需求
	//*/
	//createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
	//	VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
	///* 变换布局、复制缓冲区数据 */
	//transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
	//	VK_IMAGE_LAYOUT_UNDEFINED,
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	//copyBufferToImage(stagingBuffer, textureImage,
	//	static_cast<uint32_t>(texWidth),
	//	static_cast<uint32_t>(texHeight));
	//
	///* 为了在着色器中能采样纹理数据，在进行一次布局变换 */
	////transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
	////transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
	////	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	////	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
	//
	//vkDestroyBuffer(device, stagingBuffer, nullptr);
	//vkFreeMemory(device, stagingBufferMemory, nullptr);
	//
	//generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

	//GenerateCompressedTextureImage();
	GenerateCompressedTextureImageASTC_old();
	//GenerateCompressedTextureImageASTC();

}

/* 和createImageView差不多，只有format和image成员变量设置不同 */
void CompressedTexture::createTextureImageView() {
	//textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

void CompressedTexture::createTextureSampler() {
	/*
		该结构体指定过滤器和变换操作
		mag/minFilter 指定纹理需要方盒缩小时候使用的插值方法（放大出现采样过密，缩小采样过疏） VK_FILTER_NEAREST就是直接距离片段最近的纹理像素颜色
		U,V,W三个方向的寻址模式（变换操作，即采样超过纹理图像实际范围），有以下几种值
			- VK_SAMPLER_ADDRESS_MODE_REPEAT：采样超出图像范围时重复纹理
			- VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT：采样超出图像范围时重复镜像后的纹理
			- VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE：采样超出图像范围时使用距离最近的边界纹素
			- VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE：采样超出图像范围时使用镜像后距离最近的边界纹素
			- VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER：采样超出图像返回时返回设置的边界颜色
		anisotropyEnable:是否开启各向异性
		maxAnisotropy:限定计算最终颜色使用的样本个数，值越小采样性能越好，质量越低
		borderColor:指定使用VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER寻址模式时超过范围时返回的连接颜色
		unnormalizedCoordinates:指定采样的坐标系统：
			若为TRUE，坐标范围[0, texWidth)和[0, texHeight)
			若为FALSE，范围都为【0,1）
		compareEnable和compareOp:可以将样本和一个设定的值进行比较，并将比较结果用于之后的过滤操作，阴影贴图使用
		mipmapMode,mipLodBias,minLod,maxLod:用于设置mipmap后续解释

	*/
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	//samplerInfo.minLod = static_cast<float>(mipLevels / 2); 强制使用高级别的的mipmap
	samplerInfo.maxLod = static_cast<float>(mipLevels);

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void CompressedTexture::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageType imageType) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = imageType;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;//vulkan支持多种格式图像数据，这里用stb图像库解析的像素数据格式
	/* 若是要直接访问图像数据，应该设为LINEAR */
	imageInfo.tiling = tiling;//VK_IMAGE_TILING_LINEAR：纹素以行主序的方式排列;VK_IMAGE_TILING_OPTIMAL：纹素以一种对访问优化的方式排列
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;//后者用于-图像数据被着色器采样
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;//纸杯一个队列族使用：支持传输操作的队列族，用独占模式
	//imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;//用于设置多重采样，只对用作附着的对象有效，不用附着设置1即可
	imageInfo.samples = numSamples;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	/* 分配图像内存方法和createBuffer里分配缓冲内存几乎一模一样 */
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView CompressedTexture::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageViewType imageViewType) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = imageViewType;
	viewInfo.format = format;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;//该值实际上是0，可以忽略对components的初始化，直接设0也可以
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	//viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void CompressedTexture::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

/* 我们需要vkCmdCopyBufferToImage 但是需要图像满足一定布局要求，这里对图像布局进行变换 */
void CompressedTexture::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	/* 通过图像内存屏障image memory barrier对布局进行变换 */
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;//VK_IMAGE_LAYOUT_UNDEFINED 不需要访问之前的图像数据
	barrier.newLayout = newLayout;//VK_IMAGE_LAYOUT_PREINITIALIZED 在图像纹素变换后原数据仍存在
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;//用屏障传递队列族所有权需要对这两个成员变量设置，不传递则ignored
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	/* image和subresourceRange 用于指定进行布局变换的图像对象，收影响的图像范围。图像不存在mipmap，因此level layer为1 */
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	/* 指定在barrier之前必须发生的资源操作类型，以及必须等待barrier的资源操作类型 */
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		/* 指令缓冲的提交会隐式地进行VK_ACCESS_HOST_WRITE_BIT同步。因为transitionImageLayout函数执行的指令缓冲
		只包含了一条指令，如果布局转换依赖VK_ACCESS_HOST_WRITE_BIT，我们可以通过设置srcAccessMask的值为0来使用
		这一隐含的同步 */
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		/*
			从buffer写入图像对象，写入操作不需要等待任何对象，因此掩码为VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT，
			表示最早出现的管线阶段。

			而VK_PIPELINE_STAGE_TRANSFER_BIT并非真实存在的管线阶段，是一个伪阶段，出现在传输操作发生时
		*/
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		/* 同理，从伪阶段到片段着色器 */
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		/*
			深度缓冲数据会在进行深度测试时被读取，用来检测片段是否可以覆盖之前的片段。这一读取过程发生在
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT，如果片段可以覆盖之前的片段，新的深度缓冲数据会在
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT写入。我们应该使用与要进行的操作相匹配的管线阶段进行
			同步操作
		*/
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout ==
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}


	/*
		提交barrier对象
		1：指令缓冲对象
		2：指定发生在屏障之前的管线阶段
		3：发生在屏障之后的管线阶段，若想在一个Barrier之后读取uniform，应该指定VK_ACCESS_UNIFORM_READ_BIT标记和
		最早读取uniform的着色器阶段，比如VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		4:0 或者VK_DEPENDENCY_BY_REGION_BIT，后者表示barrier变为一个区域条件，允许我们读取资源目前已经写入的那部分
		6-12：用于引用三种可用的管线barrier数组， memory barriers（内存屏障），buffer memory barriers（缓冲内存屏障）
		和image memory barriers（图像内存屏障）
	*/
	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void CompressedTexture::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	/* 和复制缓冲区一样，用VkBufferImageCopy结构体指定将数据复制到图像哪一部分 */
	VkBufferImageCopy region = {};
	/* bufferOffset 指定要复制的数据在VkBuffer偏移，RowLength和ImageHeight 指定数据在内存中的存放方式，通过这两个成员
	可以对每行图像数据使用额外的空间进行对齐，这里都为0表示紧凑存放 */
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	/* imageSubresource imageOffset imageExtente指定数据被复制到图像的哪一部分 */
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
				width,
				height,
				1
	};

	/* 第四个参数指定目的图像当前使用的图像布局，实际上可以指定VkBufferUmageCopy数组，来一次从一个缓冲复制到不同图像对象 */
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer CompressedTexture::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void CompressedTexture::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	/* 上述步骤完成了指令缓冲的记录操作，接下来提交指令缓冲完成传输操作 */
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

/* 简化后的copy函数,why这么简化？因为我们需要copy的是图像对象，不是缓冲对象，以免重新写一个copyimagebuffer */
void CompressedTexture::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void CompressedTexture::drawFrame() {
	/* 等待一组栅栏中的一个或者全部栅栏发出信号，VK_TRUE是否等待所有fence，但是我们这里只引入了一个fence，无所谓TRUE */
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	/* 更新Uniform数据 */
	updateUniformBuffer(imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &inFlightFences[currentFrame]);//置为未发射状态


	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void CompressedTexture::updateUniformBuffer(uint32_t currentImage) {
	///* 实现绕z轴每秒转90度 */
	//static auto startTime = std::chrono::high_resolution_clock::now();
	//
	//auto currentTime = std::chrono::high_resolution_clock::now();
	///* duration表示一个时间段，第一参数代表可以传入额时间单位的类型，可以为float int等传入1.2表示1.2*period，period代表时间单位 */
	//float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	/* OpenGL和Vulkan Y轴是相反的 */
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
}

void CompressedTexture::loadModel() {
	tinyobj::attrib_t attrib;//pos,normal,texCoord
	std::vector<tinyobj::shape_t> shapes;//独立的对象name和表面数据mesh
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str())) {
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};//用于顶点去重

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],//x
				attrib.vertices[3 * index.vertex_index + 1],//y
				attrib.vertices[3 * index.vertex_index + 2] //z

			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],//U
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1] //V  Vulkan纹理坐标原点是左上角，OBJ是左下角，反转V坐标即可
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void CompressedTexture::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;/* 暂存缓冲，CPU可见的 */
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	/* 将顶点数据复制到缓冲中 */
	void* data;
	/*
		将缓冲关联的内存映射到CPU可以访问的内存，该函数允许通过给定的内存偏移值和内存大小访问特定内存资源
		偏移值：0
		内存大小：bufferinfo.size（特殊值VK_WHOLE_SIZE可以用来映射整个内存）
		倒数第二个参数：指定一个标记，目前没用处，社会为0
	*/
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	/* 复制完毕后结束映射 */
	vkUnmapMemory(device, stagingBufferMemory);

	/* 显卡读取快的真正缓冲 */
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void CompressedTexture::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);
	/* 注意用的是VK_BUFFER_USAGE_INDEX_BUFFER_BIT*/
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void CompressedTexture::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
	/*
		检查image格式是否支持线性（按行）位块传输
		VkFormatProperties包含：
		linearTilingFeatures
		optimalTilingFeatures
		bufferFeatures
		每个变量描述在对应模式下可以适用的特性，之前创建纹理Image用的是优化的tiling模式，检查第二个参数
	*/
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

	//if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
	//	throw std::runtime_error("texture image format does not support linear blitting!");
	//}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	/* 这一部分是barrier的公共部分，整个流程不变 */
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++)
	{
		/* 将i-1的纹理图像变换到SRC，这一变换在mipmap等级为i-1的纹理图像数据写入指令后进行（上一次是vkCmdCopyBufferToImage） */
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;//上一次transitionImageLayout变成了该布局
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);


		/*
			指定传输操作使用的纹理图像范围
			srcSubresource.mipLevel设为i-1就是上一级别的mipmap
			Offsets 数组用于指定要传输的数据所在的三维图像区域，二维图像深度为1
		*/
		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0,0,0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		/*
			vkCmdBlitImage指令可以进行复制，缩放和过滤图像的操作。我们会多次调用这一指令来
			生成多个不同细化级别的纹理图像数据

			!传输操作是在同一纹理额不同细化级别间进行的，因此使用同一image作为源和目的
			传输指令开始时，源细化级别刚被变化为VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			而目的级别处于创建纹理时设置的VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL，精彩
		*/
		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_NEAREST);

		/* 和之前去掉的trans一样，将SRC变换到SHADER_READ */
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;

	}
	/*
		执行指令缓冲前，插入一个barrier用于将最后一个细化图像布局由DST变为SHADER_READ，因为最后一个不会作为传输的源
		因此不会for中变换布局，需要手动变换
	*/
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

void CompressedTexture::createColorResources() {
	VkFormat colorFormat = swapChainImageFormat;

	/* VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT表示绑定的图像的内存将被懒惰的分配（按需） */
	createImage(swapChainExtent.width, swapChainExtent.height,
		1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage,
		colorImageMemory);
	colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	//同样的问题，打开会出问题
	//transitionImageLayout(colorImage, colorFormat,
	//	VK_IMAGE_LAYOUT_UNDEFINED,
	//	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
}

#include <fstream>
#include <Windows.h>
void CompressedTexture::GenerateCompressedTextureImage() {

	std::ifstream etcFile;
	etcFile.open(ETC_TEXTURE_PATH.c_str(), std::ifstream::in | std::ifstream::binary);
	// get pointer to associated buffer object
	std::filebuf* pbuf = etcFile.rdbuf();
	// get file size using buffer's memory
	std::size_t fileSize = pbuf->pubseekoff(0, etcFile.end, etcFile.in);
	pbuf->pubseekpos(0, etcFile.in);
	// allocate memory
	char* etcBuffer = new char[fileSize];
	// get file data
	pbuf->sgetn(etcBuffer, fileSize);

	etcFile.close();

	// ============etc image create ====================
	int etcWidth = 128, etcHeight = 128;
	VkDeviceSize etcImageSize = etcWidth * etcHeight * 8;

	/* 创建暂存缓冲, 存储etc数据 */
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(etcImageSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, etcImageSize, 0, &data);
	memcpy(data, etcBuffer, static_cast<size_t>(etcImageSize));
	vkUnmapMemory(device, stagingBufferMemory);


	// ============depressed image create ====================
	int texWidth = 512, texHeight = 512;
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	mipLevels = static_cast<uint32_t>(std::floor(std::log2((((texWidth) > (texHeight)) ? (texWidth) : (texHeight))))) + 1;
	createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
	// ============depressed image create ====================
	// 
	// ============depressed imageView create ====================
	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	// ============depressed imageView create ====================

	/* Create Uniform Buffer */
	VkDeviceSize BufferSize = sizeof(CompOffset);
	createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, compBuffer, compBufferMemory);
	CompOffset compOffset;
	int mBlockWidth = 4;
	int mBlockHeight = 4;
	//compOffset.offset.x = (texWidth + mBlockWidth - 1) / mBlockWidth;
	//compOffset.offset.y = (texHeight + mBlockHeight - 1) / mBlockHeight;
	compOffset.offset.x = 0;
	compOffset.offset.y = 0;
	void* data1;
	vkMapMemory(device, compBufferMemory, 0, BufferSize, 0, &data1);
	memcpy(data1, &compOffset, sizeof(compOffset));
	vkUnmapMemory(device, compBufferMemory);

	/* Load Shader */
	auto computeShaderCode = readFile("../VulkanTest/shaders/comp.spv");
	VkShaderModule compShaderModule = createShaderModule(computeShaderCode);

	/* Create Descriptor && DescriptorPool */
	VkDescriptorSetLayoutBinding dsLayoutBindings[] = {
				{
					0,                                 // bindings
					VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
					1,                            // descriptorCount
					VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
					0,                            // pImmutableSamplers
				},
				{
					1,                                 // bindings
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
					1,                            // descriptorCount
					VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
					0,                            // pImmutableSamplers
				},
				{
					2,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					1,
					VK_SHADER_STAGE_COMPUTE_BIT,
					0
				},
	};
	VkDescriptorSetLayoutCreateInfo dsLayoutInfo = {};
	dsLayoutInfo.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dsLayoutInfo.bindingCount = sizeof(dsLayoutBindings) /
		sizeof(VkDescriptorSetLayoutBinding);
	dsLayoutInfo.pBindings = dsLayoutBindings;
	if (vkCreateDescriptorSetLayout(device, &dsLayoutInfo, nullptr, &decompDescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkDescriptorPoolSize poolSize[3] = {
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
	};
	VkDescriptorPoolCreateInfo dsPoolInfo = {};
	dsPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	dsPoolInfo.flags =
		VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	dsPoolInfo.maxSets = 1;
	dsPoolInfo.poolSizeCount = 3;
	dsPoolInfo.pPoolSizes = poolSize;
	if (vkCreateDescriptorPool(device, &dsPoolInfo, nullptr, &decompDescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}

	std::vector<VkDescriptorSetLayout> layouts(1, decompDescriptorSetLayout);
	VkDescriptorSetAllocateInfo dsInfo = {};
	dsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsInfo.descriptorPool = decompDescriptorPool;
	dsInfo.descriptorSetCount = 1;
	dsInfo.pSetLayouts = layouts.data();
	decompDescriptorSets.resize(1);
	if (vkAllocateDescriptorSets(device, &dsInfo, decompDescriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	/* Create PipelineLayout */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &decompDescriptorSetLayout;
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &decompPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	/* Create Compute Pipeline */
	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computePipelineInfo.stage.module = compShaderModule;
	computePipelineInfo.stage.pName = "main";
	computePipelineInfo.layout = decompPipelineLayout;
	if (vkCreateComputePipelines(device, 0, 1, &computePipelineInfo, nullptr, &decompPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create computepipeline!");
	}

	/* Update Descriptor Sets */
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = 0;

	VkDescriptorBufferInfo inDataInfo{};
	inDataInfo.buffer = stagingBuffer;
	inDataInfo.offset = 0;
	inDataInfo.range = etcImageSize;

	VkDescriptorBufferInfo uboInfo;
	uboInfo.buffer = compBuffer;
	uboInfo.offset = 0;
	uboInfo.range = sizeof(CompOffset);

	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = decompDescriptorSets[0];
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &imageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = decompDescriptorSets[0];
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &inDataInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = decompDescriptorSets[0];
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &uboInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	//===============================================Init Done=========================================================

	/* First Image Memory Barrier */
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL, mipLevels);

	/* Do Decompress */
	{
		VkCommandBuffer decommandBuffer = beginSingleTimeCommands();
		vkCmdBindPipeline(decommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, decompPipeline);
		vkCmdBindDescriptorSets(decommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, decompPipelineLayout, 0, 1,
			decompDescriptorSets.data(), 0, nullptr);

		vkCmdDispatch(decommandBuffer, 128, 128, 1);
		endSingleTimeCommands(decommandBuffer);
	}

	/* Second Image Memory Barrier */
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_GENERAL, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

	generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SNORM, texWidth, texHeight, mipLevels);

	/* TODO: 移动至合适位置 */
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void CompressedTexture::GenerateCompressedTextureImageASTC_old() {

	std::ifstream astcFile;
	astcFile.open(ASTC_TEXTURE_PATH.c_str(), std::ifstream::in | std::ifstream::binary);
	// get pointer to associated buffer object
	std::filebuf* pbuf = astcFile.rdbuf();
	// get file size using buffer's memory
	std::size_t fileSize = pbuf->pubseekoff(0, astcFile.end, astcFile.in);
	pbuf->pubseekpos(0, astcFile.in);
	// allocate memory
	char* astcBuffer = new char[fileSize];
	// get file data
	pbuf->sgetn(astcBuffer, fileSize);

	astcFile.close();

	// ============etc image create ====================
	int texWidth = 1334, texHeight = 750;
	int astcWidth = (texWidth + 8 - 1) / 8, astcHeight = (texHeight + 8 - 1) / 8;
	VkDeviceSize astcImageSize = astcWidth * astcHeight * 16;

	/* 创建暂存缓冲, 存储astc数据 */
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(astcImageSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, astcImageSize, 0, &data);
	memcpy(data, astcBuffer, static_cast<size_t>(astcImageSize));
	vkUnmapMemory(device, stagingBufferMemory);


	// ============depressed image create ====================
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	mipLevels = static_cast<uint32_t>(std::floor(std::log2((((texWidth) > (texHeight)) ? (texWidth) : (texHeight))))) + 1;
	createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
	// ============depressed image create ====================
	// 
	// ============depressed imageView create ====================
	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	VkImageView testView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	// ============depressed imageView create ====================

	/* Create Uniform Buffer */
	//VkDeviceSize BufferSize = sizeof(CompOffset);
	//createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, compBuffer, compBufferMemory);
	//CompOffset compOffset;
	//int mBlockWidth = 4;
	//int mBlockHeight = 4;
	////compOffset.offset.x = (texWidth + mBlockWidth - 1) / mBlockWidth;
	////compOffset.offset.y = (texHeight + mBlockHeight - 1) / mBlockHeight;
	//compOffset.offset.x = 0;
	//compOffset.offset.y = 0;
	//void* data1;
	//vkMapMemory(device, compBufferMemory, 0, BufferSize, 0, &data1);
	//memcpy(data1, &compOffset, sizeof(compOffset));
	//vkUnmapMemory(device, compBufferMemory);

	/* block size */
	int mBlockW = 8;
	int mBlockH = 8;

	/* bsd buffer */
	const auto bsd = create_block_size_descriptor(mBlockW, mBlockH);
	VkBuffer bsdBuffer;
	VkDeviceMemory bsdMemory;
	size_t s = sizeof(*bsd);
	createBuffer(sizeof(*bsd), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bsdBuffer, bsdMemory);

	void* bsddata;
	vkMapMemory(device, bsdMemory, 0, sizeof(*bsd), 0, &bsddata);
	memcpy(bsddata, bsd.get(), sizeof(*bsd));
	vkUnmapMemory(device, bsdMemory);

	/* integer_table buffer */
	VkBuffer integer_tableBuffer;
	VkDeviceMemory integer_tableMemory;
	createBuffer(sizeof(TritsQuints), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, integer_tableBuffer, integer_tableMemory);

	void* integer_tabledata;
	vkMapMemory(device, integer_tableMemory, 0, sizeof(TritsQuints), 0, &integer_tabledata);
	memcpy(integer_tabledata, &TritsQuints, sizeof(TritsQuints));
	vkUnmapMemory(device, integer_tableMemory);

	/* partition_table buffer */
	const auto pi = create_partition_table(mBlockW, mBlockH);
	VkBuffer piBuffer;
	VkDeviceMemory piMemory;
	createBuffer(sizeof(*pi), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, piBuffer, piMemory);

	void* pidata;
	vkMapMemory(device, piMemory, 0, sizeof(*pi), 0, &pidata);
	memcpy(pidata, pi.get(), sizeof(*pi));
	vkUnmapMemory(device, piMemory);



	/* Load Shader */
	auto computeShaderCode = readFile("../VulkanTest/shaders/GL_COMPRESSED_RGBA_ASTC_8x8.spv");
	VkShaderModule compShaderModule = createShaderModule(computeShaderCode);

	/* test */
	shader_spv::SpvData mShaderData = loadDecompressionShader("GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8.spv");


	for (int i = 0; i < computeShaderCode.size(); ++i) {
		if (computeShaderCode[i] != (char)mShaderData.base[i]) {
			break;
		}
	}
	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.flags = 0;
	shaderInfo.codeSize = mShaderData.size;
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(mShaderData.base);
	//VkShaderModule compShaderModule;
	if (vkCreateShaderModule(device, &shaderInfo, nullptr, &compShaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}


	/* Create Descriptor && DescriptorPool */
	VkDescriptorSetLayoutBinding dsLayoutBindings[] = {
				{
					0,                                 // bindings
					VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
					1,                            // descriptorCount
					VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
					0,                            // pImmutableSamplers
				},
				{
					1,                                 // bindings
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
					1,                            // descriptorCount
					VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
					0,                            // pImmutableSamplers
				},
				{
					2,
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					1,
					VK_SHADER_STAGE_COMPUTE_BIT,
					0
				},
				{
					3,
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					1,
					VK_SHADER_STAGE_COMPUTE_BIT,
					0
				},
				{
					4,
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					1,
					VK_SHADER_STAGE_COMPUTE_BIT,
					0
				},
	};
	VkDescriptorSetLayoutCreateInfo dsLayoutInfo = {};
	dsLayoutInfo.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dsLayoutInfo.bindingCount = sizeof(dsLayoutBindings) /
		sizeof(VkDescriptorSetLayoutBinding);
	dsLayoutInfo.pBindings = dsLayoutBindings;
	if (vkCreateDescriptorSetLayout(device, &dsLayoutInfo, nullptr, &decompDescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkDescriptorPoolSize poolSize[3] = {
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
	};
	VkDescriptorPoolCreateInfo dsPoolInfo = {};
	dsPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	dsPoolInfo.flags =
		VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	dsPoolInfo.maxSets = 1;
	dsPoolInfo.poolSizeCount = 2;
	dsPoolInfo.pPoolSizes = poolSize;
	if (vkCreateDescriptorPool(device, &dsPoolInfo, nullptr, &decompDescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}

	std::vector<VkDescriptorSetLayout> layouts(1, decompDescriptorSetLayout);
	VkDescriptorSetAllocateInfo dsInfo = {};
	dsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsInfo.descriptorPool = decompDescriptorPool;
	dsInfo.descriptorSetCount = 1;
	dsInfo.pSetLayouts = layouts.data();
	decompDescriptorSets.resize(1);
	if (vkAllocateDescriptorSets(device, &dsInfo, decompDescriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	/* Create Push Constant */
	VkPushConstantRange pushConstant = {};
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(CompOffset);

	/* Create PipelineLayout */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	pipelineLayoutInfo.pSetLayouts = &decompDescriptorSetLayout;
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &decompPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	/* Create Compute Pipeline */
	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computePipelineInfo.stage.module = compShaderModule;
	computePipelineInfo.stage.pName = "main";
	computePipelineInfo.layout = decompPipelineLayout;
	if (vkCreateComputePipelines(device, 0, 1, &computePipelineInfo, nullptr, &decompPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create computepipeline!");
	}

	/* Update Descriptor Sets */
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageInfo.imageView = testView;
	imageInfo.sampler = 0;

	VkDescriptorBufferInfo inDataInfo1{};
	inDataInfo1.buffer = stagingBuffer;
	inDataInfo1.offset = 0;
	inDataInfo1.range = astcImageSize;

	VkDescriptorBufferInfo inDataInfo2{};
	inDataInfo2.buffer = bsdBuffer;
	inDataInfo2.offset = 0;
	inDataInfo2.range = sizeof(*bsd);

	VkDescriptorBufferInfo inDataInfo3{};
	inDataInfo3.buffer = integer_tableBuffer;
	inDataInfo3.offset = 0;
	inDataInfo3.range = sizeof(TritsQuints);

	VkDescriptorBufferInfo inDataInfo4{};
	inDataInfo4.buffer = piBuffer;
	inDataInfo4.offset = 0;
	inDataInfo4.range = sizeof(*pi);

	std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = decompDescriptorSets[0];
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &imageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = decompDescriptorSets[0];
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &inDataInfo1;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = decompDescriptorSets[0];
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &inDataInfo2;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = decompDescriptorSets[0];
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &inDataInfo3;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = decompDescriptorSets[0];
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &inDataInfo4;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	//===============================================Init Done=========================================================

	/* First Image Memory Barrier */
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL, mipLevels);

	CompOffset offset = {
		{0, 0},
	};

	/* Do Decompress */
	{
		VkCommandBuffer decommandBuffer = beginSingleTimeCommands();
		vkCmdBindPipeline(decommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, decompPipeline);
		vkCmdPushConstants(decommandBuffer, decompPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(offset), &offset);
		vkCmdBindDescriptorSets(decommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, decompPipelineLayout, 0, 1,
			decompDescriptorSets.data(), 0, nullptr);

		vkCmdDispatch(decommandBuffer, astcWidth, astcHeight, 1);
		
		endSingleTimeCommands(decommandBuffer);
		vkDestroyImageView(device, testView, nullptr);
		/* TODO: 移动至合适位置 */
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);

		vkDestroyBuffer(device, bsdBuffer, nullptr);
		vkFreeMemory(device, bsdMemory, nullptr);

		vkDestroyBuffer(device, integer_tableBuffer, nullptr);
		vkFreeMemory(device, integer_tableMemory, nullptr);

		vkDestroyBuffer(device, piBuffer, nullptr);
		vkFreeMemory(device, piMemory, nullptr);

	}

	/* Second Image Memory Barrier */
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

	generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SNORM, texWidth, texHeight, mipLevels);

	/* TODO: 移动至合适位置 */
	//vkDestroyBuffer(device, stagingBuffer, nullptr);
	//vkFreeMemory(device, stagingBufferMemory, nullptr);
	//
	//vkDestroyBuffer(device, bsdBuffer, nullptr);
	//vkFreeMemory(device, bsdMemory, nullptr);
	//
	//vkDestroyBuffer(device, integer_tableBuffer, nullptr);
	//vkFreeMemory(device, integer_tableMemory, nullptr);
	//
	//vkDestroyBuffer(device, piBuffer, nullptr);
	//vkFreeMemory(device, piMemory, nullptr);
}

void CompressedTexture::GenerateCompressedTextureImageASTC() {

	std::ifstream etcFile;
	etcFile.open(ASTC_TEXTURE_PATH.c_str(), std::ifstream::in | std::ifstream::binary);
	// get pointer to associated buffer object
	std::filebuf* pbuf = etcFile.rdbuf();
	// get file size using buffer's memory
	std::size_t fileSize = pbuf->pubseekoff(0, etcFile.end, etcFile.in);
	pbuf->pubseekpos(0, etcFile.in);
	// allocate memory
	char* etcBuffer = new char[fileSize];
	// get file data
	pbuf->sgetn(etcBuffer, fileSize);

	etcFile.close();


	// ============astc image create ====================
	int texWidth = 1334, texHeight = 750;
	int astcWidth = (texWidth + 8 - 1) / 8, astcHeight = (texHeight + 8 - 1) / 8;
	VkDeviceSize astcImageSize = (texWidth / 8) * (texHeight / 8) * 16;

	/* 创建暂存缓冲, 存储etc数据 */
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(astcWidth * astcHeight * 16, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, fileSize, 0, &data);
	memcpy(data, etcBuffer, static_cast<size_t>(fileSize));
	vkUnmapMemory(device, stagingBufferMemory);

	// ============raw image create ====================

	createImage(astcWidth, astcHeight, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, astcImage, astcImageMemory);

	astcImageView = createImageView(astcImage, VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

	transitionImageLayout(astcImage, VK_FORMAT_R32G32B32A32_UINT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
	copyBufferToImage(stagingBuffer, astcImage,
		static_cast<uint32_t>(astcWidth),
		static_cast<uint32_t>(astcHeight));
	
	transitionImageLayout(astcImage, VK_FORMAT_R32G32B32A32_UINT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL, 1);
	
	//vkDestroyBuffer(device, stagingBuffer, nullptr);
	//vkFreeMemory(device, stagingBufferMemory, nullptr);

	// ============raw image create ====================

	// ============depressed image create ====================
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	mipLevels = static_cast<uint32_t>(std::floor(std::log2((((texWidth) > (texHeight)) ? (texWidth) : (texHeight))))) + 1;
	createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
	// ============depressed image create ====================	

	// ============depressed imageView create ====================
	computeTextureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
	// ============depressed imageView create ====================


	/* Load Shader */
	auto computeShaderCode = readFile("../VulkanTest/shaders/Astc_2DArray.spv");
	VkShaderModule compShaderModule = createShaderModule(computeShaderCode);

	/* Create Descriptor && DescriptorPool */
	VkDescriptorSetLayoutBinding dsLayoutBindings[] = {
				{
					0,                                 // bindings
					VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
					1,                            // descriptorCount
					VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
					0,                            // pImmutableSamplers
				},
				{
					1,                                 // bindings
					VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
					1,                            // descriptorCount
					VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
					0,                            // pImmutableSamplers
				},
	};
	VkDescriptorSetLayoutCreateInfo dsLayoutInfo = {};
	dsLayoutInfo.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dsLayoutInfo.bindingCount = sizeof(dsLayoutBindings) /
		sizeof(VkDescriptorSetLayoutBinding);
	dsLayoutInfo.pBindings = dsLayoutBindings;
	if (vkCreateDescriptorSetLayout(device, &dsLayoutInfo, nullptr, &decompDescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkDescriptorPoolSize poolSize[1] = {
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2},
	};
	VkDescriptorPoolCreateInfo dsPoolInfo = {};
	dsPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	dsPoolInfo.flags =
		VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	dsPoolInfo.maxSets = 1;
	dsPoolInfo.poolSizeCount = 1;
	dsPoolInfo.pPoolSizes = poolSize;
	if (vkCreateDescriptorPool(device, &dsPoolInfo, nullptr, &decompDescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}

	std::vector<VkDescriptorSetLayout> layouts(1, decompDescriptorSetLayout);
	VkDescriptorSetAllocateInfo dsInfo = {};
	dsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsInfo.descriptorPool = decompDescriptorPool;
	dsInfo.descriptorSetCount = 1;
	dsInfo.pSetLayouts = layouts.data();
	decompDescriptorSets.resize(1);
	if (vkAllocateDescriptorSets(device, &dsInfo, decompDescriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	/* Create Push Constant */
	VkPushConstantRange pushConstant = {};
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(AstcPushConstant);

	/* Create PipelineLayout */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &decompDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &decompPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	/* Create Compute Pipeline */
	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computePipelineInfo.stage.module = compShaderModule;
	computePipelineInfo.stage.pName = "main";
	computePipelineInfo.layout = decompPipelineLayout;
	if (vkCreateComputePipelines(device, 0, 1, &computePipelineInfo, nullptr, &decompPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create computepipeline!");
	}

	/* Update Descriptor Sets */
	VkDescriptorImageInfo astcimageInfo{};
	astcimageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	astcimageInfo.imageView = astcImageView;
	astcimageInfo.sampler = 0;

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageInfo.imageView = computeTextureImageView;
	imageInfo.sampler = 0;

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = decompDescriptorSets[0];
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &astcimageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = decompDescriptorSets[0];
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	//===============================================Init Done=========================================================

	/* First Image Memory Barrier */
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UINT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL, mipLevels);

	uint32_t srgb = false;
	uint32_t smallBlock = false;
	AstcPushConstant astcPushConstant = {
		{8, 8},
		static_cast<uint32_t>(VK_FORMAT_ASTC_8x8_UNORM_BLOCK),
		0,
		srgb,
		smallBlock,
	};

	/* Do Decompress */
	{
		VkCommandBuffer decommandBuffer = beginSingleTimeCommands();
		vkCmdBindPipeline(decommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, decompPipeline);
		vkCmdPushConstants(decommandBuffer, decompPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(astcPushConstant), &astcPushConstant);
		vkCmdBindDescriptorSets(decommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, decompPipelineLayout, 0, 1,
			decompDescriptorSets.data(), 0, nullptr);

		vkCmdDispatch(decommandBuffer, astcWidth, astcHeight, 1);
		endSingleTimeCommands(decommandBuffer);
	}

	/* Second Image Memory Barrier */
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UINT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

	generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_UINT, texWidth, texHeight, mipLevels);

	/* TODO: 移动至合适位置 */
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

shader_spv::SpvData CompressedTexture::loadDecompressionShader(const char* formatName)
{
	size_t numDecompressionShader =
		sizeof(shader_spv::sDecompressionShaderData) / sizeof(shader_spv::sDecompressionShaderData[0]);
	for (size_t i = 0; i < numDecompressionShader; ++i)
	{
		if (!strcmp(formatName, shader_spv::sDecompressionShaderData[i].name))
		{
			return shader_spv::sDecompressionShaderData[i];
		}
	}

	shader_spv::SpvData invalid = {
		formatName, nullptr, 0,
	};

	

	return invalid;
}
