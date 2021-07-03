#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1 

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "VulkanResources.h"
#include "Game.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
	void* pUserData);
std::vector<char const*> getRequiredExtensions();
bool					checkValidationLayerSupport();
void					populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);
bool					isDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface);
QueueFamilyIndices		findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);
bool					checkDeviceExtensionSupport(vk::PhysicalDevice device);
SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);
vk::SurfaceFormatKHR	chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
vk::PresentModeKHR		chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);
vk::Extent2D			chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, GLFWwindow* window);
vk::Format				findDepthFormat(vk::PhysicalDevice physicalDevice);
vk::Format				findSupportedFormat(std::vector<vk::Format> const& candidates, vk::ImageTiling tiling,
	vk::FormatFeatureFlags features, vk::PhysicalDevice physicalDevice);
vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice);
vk::ShaderModule		createShaderModule(std::vector<char> const& code, vk::Device device);
uint32_t				findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice physicalDevice);
vk::CommandBuffer		beginSingleTimeCommands(vk::Device device, vk::CommandPool commandPool);
void					endSingleTimeCommands(vk::CommandBuffer commandBuffer, vk::Device device, vk::CommandPool commandPool, vk::Queue graphicsQueue);
bool					hasStencilComponent(vk::Format format);
void					generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth,
	int32_t texHeight, uint32_t mipLevels, vk::Device device, vk::CommandPool commandPool,
	vk::PhysicalDevice physicalDevice, vk::Queue graphicsQueue);
void					loadModel(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
void					copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size,
	vk::Device device, vk::CommandPool commandPool, vk::Queue graphicsQueue);

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

VulkanResources::VulkanResources(Game* game)
	:vertShaderPath{ Settings::VERT_SHADER_PATH }, fragShaderPath{ Settings::FRAG_SHADER_PATH },
	game{ game }
{
	initWindow();

	initVulkan();
}

VulkanResources::~VulkanResources()
{
	cleanupSwapChain();
	std::cout << "finished cleaning up swapchain\n";

	device.destroySampler(textureSampler);
	std::cout << "destroyed texture sampler\n";

	device.destroyDescriptorSetLayout(descriptorSetLayout, nullptr);
	std::cout << "destroyed descriptor set layout\n";

	device.destroyBuffer(indexBuffer);
	device.freeMemory(indexBufferMemory);
	std::cout << "destroyed index buffer and freed memory\n";

	device.destroyBuffer(vertexBuffer);
	device.freeMemory(vertexBufferMemory);
	std::cout << "destroyed vertex buffer and freed memory\n";

	for (size_t i = 0; i < Settings::MAX_FRAMES_IN_FLIGHT; i++)
	{
		device.destroySemaphore(renderFinishedSemaphores[i]);
		device.destroySemaphore(imageAvailableSemaphores[i]);
		device.destroyFence(inFlightFences[i]);
	}
	std::cout << "destroyed " << Settings::MAX_FRAMES_IN_FLIGHT << " image available semaphores, render finished semaphores and in flight fences\n";

	device.destroyCommandPool(commandPool, nullptr);
	std::cout << "destroyed command pool\n";

	device.destroy();
	std::cout << "destroyed device\n";

	if (enableValidationLayers)
	{
		instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr);
		std::cout << "destroyed debug messenger\n";
	}

	instance.destroySurfaceKHR(surface);
	std::cout << "destroyed surface\n";
	instance.destroy();
	std::cout << "destroyed instance\n";

	glfwDestroyWindow(window);
	std::cout << "destroyed glfw window\n";

	glfwTerminate();
	std::cout << "terminated glfw\n";
}

void VulkanResources::initWindow()
{
	glfwInit();

	//do not create an opengl context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(Settings::WINDOW_WIDTH, Settings::WINDOW_HEIGHT, "Vulkaners: Rebirth",
		nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	//sets bool in game that window was resized
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

//loader for vulkan functions
void VulkanResources::createDynamicLoader()
{
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
}

void VulkanResources::initVulkan()
{
	createDynamicLoader();
	createInstance();
	//load instance specific functions
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	//load device specific functions
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createTextures();
	createTextureSampler();
	loadModel(vertices, indices);
	createVertexBuffer();
	createIndexBuffer();
	createDescriptorPool();
	spritesToRender = std::make_unique<SpritePool>(this);
	createCommandBuffers();
	createSyncObjects();
}

void VulkanResources::createInstance()
{
#pragma warning(suppress : 6237)
	if (enableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available");
	}

	vk::ApplicationInfo appInfo("Totris", VK_MAKE_VERSION(1, 0, 0),
		"No Engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_2);

	auto glfwExtensions = getRequiredExtensions();

	vk::InstanceCreateInfo instanceInfo({}, &appInfo, {}, {}, (uint32_t)glfwExtensions.size(),
		glfwExtensions.data());

	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	//whether or not to create a debug messenger
	if (enableValidationLayers)
	{
		instanceInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		instanceInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);

		instanceInfo.pNext = &debugCreateInfo;
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;

		instanceInfo.pNext = nullptr;
	}

	vk::createInstance(&instanceInfo, nullptr, &instance);
	std::cout << "created Vulkan instance\n";
}

void VulkanResources::setupDebugMessenger()
{
	if (!enableValidationLayers)
	{
		return;
	}

	vk::DebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	instance.createDebugUtilsMessengerEXT(&createInfo, nullptr, &debugMessenger);
	std::cout << "created debug messenger\n";
}

void VulkanResources::createSurface()
{
	VkSurfaceKHR createdSurface;
	if (glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &createdSurface) != VK_SUCCESS)
	{
		throw std::runtime_error("couldn't create window surface");
	}
	surface = vk::SurfaceKHR(createdSurface);
	std::cout << "created window surface\n";
}

void VulkanResources::pickPhysicalDevice()
{
	physicalDevice = vk::PhysicalDevice(nullptr);

	std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

	if (devices.empty())
	{
		throw std::runtime_error("Couldn't find GPUs with Vulkan support");
	}

	std::cout << "devices with vulkan support: \n";
	for (auto const& device : devices)
	{
		vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
		std::cout << "\t" << deviceProperties.deviceName << "\n";
	}

	for (auto const& device : devices)
	{
		if (isDeviceSuitable(device, surface))
		{
			physicalDevice = device;
			std::cout << "multisampling is disabled\n";
			msaaSamples = vk::SampleCountFlagBits::e1;
			break;
		}
	}

	if (!physicalDevice)
	{
		throw std::runtime_error("failed to find a suitable GPU");
	}
}

void VulkanResources::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	//create graphics and present queues
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::unordered_set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(),
		indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamily, 1, &queuePriority);
		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	std::cout << "sampler anisotropy is enabled\n";

	if (msaaSamples != vk::SampleCountFlagBits::e1)
	{
		deviceFeatures.sampleRateShading = VK_TRUE;
		std::cout << "sample rate shading is enabled\n";
	}

	vk::DeviceCreateInfo createInfo({}, (uint32_t)queueCreateInfos.size(), queueCreateInfos.data(),
		{}, {}, (uint32_t)deviceExtensions.size(), deviceExtensions.data(), &deviceFeatures);

	//add debug info layers
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = (uint32_t)(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	device = physicalDevice.createDevice(createInfo, nullptr);

	std::cout << "created logical device\n";

	graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
	std::cout << "created graphics queue\n";
	presentQueue = device.getQueue(indices.presentFamily.value(), 0);
	std::cout << "created present queue\n";
}

void VulkanResources::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

	vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	std::cout << "maximum swap chain images: " << swapChainSupport.capabilities.maxImageCount << "\n";
	std::cout << "chosen swap chain image count: " << imageCount << "\n";

	vk::SwapchainCreateInfoKHR createInfo({}, surface, imageCount, surfaceFormat.format,
		surfaceFormat.colorSpace, extent, 1, vk::ImageUsageFlagBits::eColorAttachment);

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
		std::cout << "image sharing mode between queue families is concurrent\n";
	}
	else
	{
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		std::cout << "image sharing mode between queue families is exclusive (graphics and present families are the same)\n";
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = nullptr;

	swapChain = device.createSwapchainKHR(createInfo, nullptr);
	std::cout << "created swap chain\n";

	swapChainImages = device.getSwapchainImagesKHR(swapChain);
	std::cout << "created " << swapChainImages.size() << " swap chain images\n";

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void VulkanResources::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, vk::ImageAspectFlagBits::eColor, 1);
	}
	std::cout << "created " << swapChainImageViews.size() << " swap chain image views\n";
}

vk::ImageView VulkanResources::createImageView(vk::Image image, vk::Format format,
	vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	vk::ImageSubresourceRange subresourceRange(aspectFlags, 0, mipLevels, 0, 1);

	vk::ImageViewCreateInfo viewInfo({}, image, vk::ImageViewType::e2D, format,
		{}, subresourceRange);

	vk::ImageView imageView = device.createImageView(viewInfo, nullptr);
	return imageView;
}

void VulkanResources::createRenderPass()
{
	vk::AttachmentDescription colorAttachment({}, swapChainImageFormat,
		msaaSamples, vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR);

	vk::Format depthFormat = findDepthFormat(physicalDevice);
	switch (depthFormat)
	{
	case vk::Format::eD32Sfloat:
		std::cout << "render pass depth format is D32Sfloat\n";
		break;
	case vk::Format::eD32SfloatS8Uint:
		std::cout << "render pass depth format is D32SfloatS8Uint\n";
		break;
	case vk::Format::eD24UnormS8Uint:
		std::cout << "render pass depth format is D24UnormS8Uint\n";
		break;
	default:
		std::cout << "render pass depth format is unknown\n";
		break;
	}
	vk::AttachmentDescription depthAttachment({}, depthFormat,
		msaaSamples, vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, {}, 1,
		&colorAttachmentRef, {}, &depthAttachmentRef);

	std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment,
		depthAttachment };

	vk::RenderPassCreateInfo renderPassInfo({}, (uint32_t)attachments.size(),
		attachments.data(), 1, &subpass);

	vk::SubpassDependency dependency(VK_SUBPASS_EXTERNAL, 0,
		vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		{}, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead);

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	renderPass = device.createRenderPass(renderPassInfo, nullptr);
	std::cout << "created render pass\n";
}

void VulkanResources::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding uboLayoutBinding(0, vk::DescriptorType::eUniformBuffer,
		1, vk::ShaderStageFlagBits::eVertex, nullptr);

	vk::DescriptorSetLayoutBinding samplerLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler,
		1, vk::ShaderStageFlagBits::eFragment, nullptr);

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	vk::DescriptorSetLayoutCreateInfo layoutInfo({}, (uint32_t)bindings.size(), bindings.data());

	descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo, nullptr);
	std::cout << "created descriptor set layout\n";
}

void VulkanResources::createGraphicsPipeline()
{
	auto vertShaderCode = readFile(vertShaderPath);
	auto fragShaderCode = readFile(fragShaderPath);

	vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode, device);
	std::cout << "created vertex shader module\n";
	vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode, device);
	std::cout << "created fragment shader module\n";

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo({},
		vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main");

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo({},
		vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main");

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 1, &bindingDescription,
		(uint32_t)attributeDescriptions.size(), attributeDescriptions.data());

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly({},
		vk::PrimitiveTopology::eTriangleList, VK_FALSE);

	vk::Viewport viewport(0.0f, 0.0f, (float)swapChainExtent.width,
		(float)swapChainExtent.height, 0.0f, 1.0f);

	vk::Rect2D scissor({ 0, 0 }, swapChainExtent);

	vk::PipelineViewportStateCreateInfo viewPortState({}, 1, &viewport, 1, &scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizer({}, VK_FALSE, VK_FALSE,
		vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise,
		VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);

	vk::PipelineMultisampleStateCreateInfo multisampling({}, msaaSamples, VK_FALSE, 0.0f,
		nullptr, VK_FALSE, VK_FALSE);

	vk::PipelineColorBlendAttachmentState colorBlendAttachment(VK_TRUE, vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero,
		vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	vk::PipelineColorBlendStateCreateInfo colorBlending({}, VK_FALSE, vk::LogicOp::eCopy,
		1, &colorBlendAttachment, { 0.0f, 0.0f, 0.0f, 0.0f });

	vk::PipelineDepthStencilStateCreateInfo depthStencil({}, VK_TRUE, VK_TRUE,
		vk::CompareOp::eLess, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f);

	vk::PushConstantRange range(vk::ShaderStageFlagBits::eFragment, 0, 2 * sizeof(float));

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 1, &descriptorSetLayout, 1, &range);

	pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo, nullptr);
	std::cout << "created pipeline layout\n";

	vk::GraphicsPipelineCreateInfo pipelineInfo({}, 2, shaderStages, &vertexInputInfo,
		&inputAssembly, nullptr, &viewPortState, &rasterizer, &multisampling, &depthStencil,
		&colorBlending, nullptr, pipelineLayout, renderPass, 0, vk::Pipeline(nullptr), -1);

	graphicsPipeline = device.createGraphicsPipelines(vk::PipelineCache(nullptr), pipelineInfo, nullptr).value[0];
	std::cout << "created graphics pipeline\n";

	device.destroyShaderModule(fragShaderModule, nullptr);
	device.destroyShaderModule(vertShaderModule, nullptr);
	std::cout << "destroyed shader modules\n";
}

void VulkanResources::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient,
		queueFamilyIndices.graphicsFamily.value());
	commandPool = device.createCommandPool(poolInfo, nullptr);
	std::cout << "created command pool\n";
}

void VulkanResources::createColorResources()
{
	vk::Format colorFormat = swapChainImageFormat;

	createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples,
		colorFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment |
		vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
		colorImage, colorImageMemory);
	colorImageView = createImageView(colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
	std::cout << "created color resources\n";
}

void VulkanResources::createDepthResources()
{
	vk::Format depthFormat = findDepthFormat(physicalDevice);

	createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat,
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
	std::cout << "created depth resources\n";

	transitionImageLayout(depthImage, depthFormat, vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
	std::cout << "transitioned depth image layout from \"undefined\" to \"depth stencil attachment optimal\"\n";
}

void VulkanResources::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		std::array<vk::ImageView, 2> attachments = {
			swapChainImageViews[i],
			depthImageView
		};

		vk::FramebufferCreateInfo framebufferInfo({}, renderPass, (uint32_t)attachments.size(),
			attachments.data(), (uint32_t)swapChainExtent.width, (uint32_t)swapChainExtent.height, 1);

		swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
	}

	std::cout << "created " << swapChainImageViews.size() << " framebuffers\n";
}

void VulkanResources::createTextures()
{
	textures[0] = Texture("textures/chess.png", this);
	textures[1] = Texture("textures/white pawn.png", this);
	textures[2] = Texture("textures/black pawn.png", this);
	textures[3] = Texture("textures/white rook.png", this);
	textures[4] = Texture("textures/black rook.png", this);
	textures[5] = Texture("textures/white knight.png", this);
	textures[6] = Texture("textures/black knight.png", this);
	textures[7] = Texture("textures/white bishop.png", this);
	textures[8] = Texture("textures/black bishop.png", this);
	textures[9] = Texture("textures/white queen.png", this);
	textures[10] = Texture("textures/black queen.png", this);
	textures[11] = Texture("textures/white king.png", this);
	textures[12] = Texture("textures/black king.png", this);
	textures[13] = Texture("textures/cursor.png", this);
	textures[14] = Texture("textures/outline_blue.png", this);
	textures[15] = Texture("textures/outline_green.png", this);
	textures[16] = Texture("textures/highlight_green.png", this);
	textures[17] = Texture("textures/highlight_red.png", this);
}

void VulkanResources::createTextureSampler()
{
	vk::SamplerCreateInfo samplerInfo({}, vk::Filter::eLinear, vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge, 0.0f, VK_TRUE, 16, VK_FALSE, vk::CompareOp::eAlways,
		0.0f, 1.0f, vk::BorderColor::eFloatTransparentBlack, VK_FALSE);

	textureSampler = device.createSampler(samplerInfo);
	std::cout << "created texture sampler\n";
}

void VulkanResources::createVertexBuffer()
{
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		stagingBuffer, stagingBufferMemory);
	std::cout << "created staging buffer for vertex data\n";

	void* data;
	data = device.mapMemory(stagingBufferMemory, 0, bufferSize, {});
	memcpy(data, vertices.data(), (size_t)bufferSize);
	device.unmapMemory(stagingBufferMemory);
	std::cout << "copied vertex data to staging buffer\n";

	createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);
	std::cout << "created vertex buffer\n";

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize, device, commandPool, graphicsQueue);
	std::cout << "copied data from staging buffer to vertex buffer\n";

	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingBufferMemory);
	std::cout << "destroyed staging buffer and freed memory\n";
}

void VulkanResources::createIndexBuffer()
{
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		stagingBuffer, stagingBufferMemory);
	std::cout << "created staging buffer for index data\n";

	void* data;
	data = device.mapMemory(stagingBufferMemory, 0, bufferSize, {});
	memcpy(data, indices.data(), (size_t)bufferSize);
	device.unmapMemory(stagingBufferMemory);
	std::cout << "copied index data to staging buffer\n";

	createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);
	std::cout << "created index buffer\n";

	copyBuffer(stagingBuffer, indexBuffer, bufferSize, device, commandPool, graphicsQueue);
	std::cout << "copied data from staging buffer to index buffer\n";

	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingBufferMemory);
	std::cout << "destroyed staging buffer and freed memory\n";
}

void VulkanResources::createDescriptorPool()
{
	std::array<vk::DescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].descriptorCount = (uint32_t)swapChainImages.size() * 512;
	poolSizes[1].descriptorCount = (uint32_t)swapChainImages.size() * 512;

	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		(uint32_t)swapChainImages.size() * 512, (uint32_t)poolSizes.size(), poolSizes.data());

	descriptorPool = device.createDescriptorPool(poolInfo);
	std::cout << "created descriptor pool\n";
}


void VulkanResources::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());

	vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary,
		(uint32_t)commandBuffers.size());

	commandBuffers = device.allocateCommandBuffers(allocInfo);
	std::cout << "allocated " << commandBuffers.size() << " command buffers\n";

	for (uint32_t i = 0; i < commandBuffers.size(); i++)
	{
		updateCommandBuffer(i);
	}
	std::cout << "recorded " << commandBuffers.size() << " command buffers\n";
}

void VulkanResources::createSyncObjects()
{
	imageAvailableSemaphores.resize(Settings::MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(Settings::MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(Settings::MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapChainImages.size(), nullptr);

	vk::SemaphoreCreateInfo semaphoreInfo{};

	vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

	for (size_t i = 0; i < Settings::MAX_FRAMES_IN_FLIGHT; i++)
	{
		imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
		renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
		inFlightFences[i] = device.createFence(fenceInfo);
	}
	std::cout << "created " << Settings::MAX_FRAMES_IN_FLIGHT << " image available semaphores, render finished semaphores and in flight fences\n";
}

void VulkanResources::drawFrame()
{
	device.waitForFences(inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	vk::ResultValue<uint32_t> resultValue(vk::Result::eSuccess, 0);
	try
	{
		resultValue = device.acquireNextImageKHR(swapChain, UINT64_MAX,
			imageAvailableSemaphores[currentFrame], nullptr);
		imageIndex = resultValue.value;
	}
	catch (std::exception const& e)
	{
		if (strcmp(e.what(), "vk::Device::acquireNextImageKHR: ErrorOutOfDateKHR"))
		{
			recreateSwapChain();
			return;
		}
		else
		{
			throw std::runtime_error("couldn't acquire swap chain image\n");
		}
	}

	if (imagesInFlight[imageIndex])
	{
		device.waitForFences(imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	updateSprites(imageIndex);

	updateCommandBuffer(imageIndex);

	std::vector<vk::Semaphore> waitSemaphores = { imageAvailableSemaphores[currentFrame] };
	std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	std::vector<vk::Semaphore> signalSemaphores = { renderFinishedSemaphores[currentFrame] };

	vk::SubmitInfo submitInfo(1, waitSemaphores.data(), waitStages.data(), 1,
		&commandBuffers[imageIndex], 1, signalSemaphores.data());

	device.resetFences(1, &inFlightFences[currentFrame]);

	graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);

	std::vector<vk::SwapchainKHR> swapChains = { swapChain };
	vk::PresentInfoKHR presentInfo(1, signalSemaphores.data(), 1, swapChains.data(),
		&imageIndex, nullptr);

	try
	{
		resultValue.result = presentQueue.presentKHR(presentInfo);
	}
	catch (std::exception const& e)
	{
		if (strcmp(e.what(), "vk::Queue::presentKHR: ErrorOutOfDateKHR") == 0)
		{
			recreateSwapChain();
		}
		else
		{
			throw std::runtime_error("couldn't present swap chain image\n");
		}
	}
	if (resultValue.result == vk::Result::eSuboptimalKHR)
	{
		recreateSwapChain();
	}

	currentFrame = (currentFrame + 1) % Settings::MAX_FRAMES_IN_FLIGHT;
}

void VulkanResources::cleanupSwapChain()
{
	for (auto i = 0; i < commandBuffers.size(); i++)
	{
		device.freeCommandBuffers(commandPool, commandBuffers[i]);
	}

	device.destroyImageView(colorImageView);
	device.destroyImage(colorImage);
	device.freeMemory(colorImageMemory);
	std::cout << "destroyed color image, view, and freed memory\n";

	device.destroyImageView(depthImageView);
	device.destroyImage(depthImage);
	device.freeMemory(depthImageMemory);
	std::cout << "destroyed depth image, view, and freed memory\n";

	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		device.destroyFramebuffer(swapChainFramebuffers[i]);
	}
	std::cout << "destroyed " << swapChainFramebuffers.size() << " framebuffers\n";

	device.destroyPipeline(graphicsPipeline, nullptr);
	device.destroyPipelineLayout(pipelineLayout, nullptr);
	std::cout << "destroyed pipeline and pipeline layout\n";

	device.destroyRenderPass(renderPass, nullptr);
	std::cout << "destroyed render pass\n";

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		device.destroyImageView(swapChainImageViews[i], nullptr);
	}
	std::cout << "destroyed " << swapChainImageViews.size() << " swap chain image views\n";

	device.destroySwapchainKHR(swapChain, nullptr);
	std::cout << "destroyed swapchain\n";

	spritesToRender.reset(nullptr);

	device.destroyDescriptorPool(descriptorPool);
	std::cout << "destroyed descriptor pool\n";
}

void VulkanResources::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	device.waitIdle();

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createDescriptorPool();
	spritesToRender = std::make_unique<SpritePool>(this);
	createSprites();
	createCommandBuffers();
}

void VulkanResources::createSprites()
{
	game->recreateSprites();
}

void VulkanResources::updateSprites(uint32_t imageIndex)
{
	spritesToRender->update(imageIndex);
}

void VulkanResources::updateCommandBuffer(uint32_t imageIndex)
{
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr);
	commandBuffers[imageIndex].begin(beginInfo);

	vk::Rect2D renderArea({ 0,0 }, swapChainExtent);
	std::array<vk::ClearValue, 2> clearValues = {};
	clearValues[0].color = vk::ClearColorValue{ std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
	vk::RenderPassBeginInfo renderPassInfo(renderPass, swapChainFramebuffers[imageIndex],
		renderArea, (uint32_t)clearValues.size(), clearValues.data());

	commandBuffers[imageIndex].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	commandBuffers[imageIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

	vk::DeviceSize offsets = { 0 };
	commandBuffers[imageIndex].bindVertexBuffers(0, vertexBuffer, offsets);

	commandBuffers[imageIndex].bindIndexBuffer(indexBuffer, offsets, vk::IndexType::eUint32);

	//get sprite array from sprite pool
	auto const& sprites = spritesToRender->getSprites();
	for (int i = 0; i < spritesToRender->size(); i++)
	{
		if (!sprites[i].isRemoved)
		{
			commandBuffers[imageIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout,
				0, sprites[i].descriptorSets[imageIndex], nullptr);

			commandBuffers[imageIndex].drawIndexed((uint32_t)indices.size(), 1, 0, 0, 0);
		}
	}
	commandBuffers[imageIndex].endRenderPass();
	commandBuffers[imageIndex].end();
}

short VulkanResources::addSprite(float posX, float posY, SpriteLayers layer, float sizeX, float sizeY, float rotation,
	vk::Sampler sampler, Texture* texture, GraphicsComponent* object)
{
	return spritesToRender->addSprite(posX, posY, layer, sizeX, sizeY, rotation, sampler, texture, object);
}

void VulkanResources::removeSprite(unsigned short index)
{
	spritesToRender->removeSprite(index);
}

bool checkValidationLayerSupport()
{
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

	std::cout << "available layers: \n";
	for (auto const& layerProperties : availableLayers)
	{
		std::cout << "\t" << layerProperties.layerName << "\n";
	}

	for (auto layerName : validationLayers)
	{
		bool layerFound = false;
		for (auto const& layerProperties : availableLayers)
		{
			if (strcmp(layerProperties.layerName, layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			std::cout << layerName << " layer couldn't be found\n";
			return false;
		}
		else
		{
			std::cout << layerName << " layer is active\n";
		}
	}

	return true;
}

std::vector<char const*> getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<char const*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
	std::cout << "available extensions: \n";

	for (auto const& extension : availableExtensions)
	{
		std::cout << "\t" << extension.extensionName << "\n";
	}

	std::cout << "Total required extensions: " << extensions.size() << "\n";

	for (int i = 0; i < extensions.size(); i++)
	{
		bool extension_present = false;
		for (auto const& extension : availableExtensions)
		{
			if (strcmp(extension.extensionName, extensions[i]) == 0)
			{
				extension_present = true;
				break;
			}
		}
		if (extension_present)
		{
			std::cout << "\t" << extensions[i] << " is supported\n";
		}
		else
		{
			std::string errorMessage = extensions[i];
			errorMessage += " is not supported";
			throw std::runtime_error(errorMessage);
		}
	}

	return extensions;
}

void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = vk::DebugUtilsMessengerCreateInfoEXT();
	createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
	createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
	void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << "\n";

	return VK_FALSE;
}

//sets bool in Game
static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto program = reinterpret_cast<VulkanResources*>(glfwGetWindowUserPointer(window));
	program->game->setWindowWasResized();
}

bool isDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
	vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
	std::cout << deviceProperties.deviceName << " is being queried\n";

	std::cout << deviceProperties.limits.maxVertexInputBindings << " max vertex input bindings\n";
	std::cout << deviceProperties.limits.maxDescriptorSetUniformBuffers << " max descriptor set uniform buffers\n";
	std::cout << deviceProperties.limits.maxMemoryAllocationCount << " max memory allocation count\n";
	std::cout << deviceProperties.vendorID << " vendor id\n";
	if (deviceProperties.vendorID == 0x8086)
	{
		return false;
	}

	QueueFamilyIndices indices = findQueueFamilies(device, surface);

	std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
	std::cout << "Total number of queue families: " << queueFamilies.size() << "\n";

	if (indices.isComplete())
	{
		std::cout << "graphics queue family index: " << indices.graphicsFamily.value() << "\n";
		std::cout << "present queue family index: " << indices.presentFamily.value() << "\n";
		std::cout << "Number of graphics queues: " << queueFamilies[indices.graphicsFamily.value()].queueCount << "\n";
		std::cout << "Number of present queues: " << queueFamilies[indices.presentFamily.value()].queueCount << "\n";
	}

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();
	if (supportedFeatures.samplerAnisotropy)
	{
		std::cout << "sampler anisotropy is supported\n";
	}
	else
	{
		std::cout << "sampler anisotropy is not supported\n";
	}

	bool deviceIsSuitable = indices.isComplete() && extensionsSupported &&
		swapChainAdequate && supportedFeatures.samplerAnisotropy;

	if (deviceIsSuitable)
	{
		std::cout << deviceProperties.deviceName << " was picked as a suitable GPU\n";
	}

	return deviceIsSuitable;
}

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
	QueueFamilyIndices indices;

	std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

	int i = 0;
	for (auto const& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		presentSupport = device.getSurfaceSupportKHR(i, surface);
		if (presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice device)
{
	std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

	std::cout << "supported extensions: \n";
	for (auto const& extension : availableExtensions)
	{
		std::cout << "\t" << extension.extensionName << "\n";
	}

	std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (auto const& extension : availableExtensions)
	{
		if (requiredExtensions.erase(extension.extensionName) == 1)
		{
			std::cout << extension.extensionName << " is supported\n";
		}
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
	SwapChainSupportDetails details;

	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);

	details.formats = device.getSurfaceFormatsKHR(surface);

	details.presentModes = device.getSurfacePresentModesKHR(surface);

	return details;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
	for (auto const& availableFormat : availableFormats)
	{
		if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
			availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			std::cout << "swap surface format is B8G8R8A8Unorm with SRGB color space supported\n";
			return availableFormat;
		}
	}

	std::cout << "swap surface format is " << (int)availableFormats[0].format <<
		" with " << (int)availableFormats[0].colorSpace << " color space\n";

	return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
{
	for (auto const& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == vk::PresentModeKHR::eMailbox)
		{
			std::cout << "swap present mode is Mailbox\n";
			return availablePresentMode;
		}
	}

	std::cout << "swap present mode is FIFO\n";
	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, GLFWwindow* window)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		std::cout << "swap extent is " << capabilities.currentExtent.width <<
			"x" << capabilities.currentExtent.height << "\n";
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		vk::Extent2D actualExtent = {
			(uint32_t)(width),
			(uint32_t)(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height));

		std::cout << "swap extent is " << actualExtent.width <<
			"x" << actualExtent.height << "\n";
		return actualExtent;
	}
}

vk::Format findDepthFormat(vk::PhysicalDevice physicalDevice)
{
	return findSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment, physicalDevice);
}

vk::Format findSupportedFormat(std::vector<vk::Format> const& candidates, vk::ImageTiling tiling,
	vk::FormatFeatureFlags features, vk::PhysicalDevice physicalDevice)
{
	for (vk::Format format : candidates)
	{
		vk::FormatProperties props = physicalDevice.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("couldn't find supported format");
}

vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice)
{
	vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();

	vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
		physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & vk::SampleCountFlagBits::e64)
	{
		std::cout << "max msaa samples are 64\n";
		return vk::SampleCountFlagBits::e64;
	}
	if (counts & vk::SampleCountFlagBits::e32)
	{
		std::cout << "max msaa samples are 32\n";
		return vk::SampleCountFlagBits::e32;
	}
	if (counts & vk::SampleCountFlagBits::e16)
	{
		std::cout << "max msaa samples are 16\n";
		return vk::SampleCountFlagBits::e16;
	}
	if (counts & vk::SampleCountFlagBits::e8)
	{
		std::cout << "max msaa samples are 8\n";
		return vk::SampleCountFlagBits::e8;
	}
	if (counts & vk::SampleCountFlagBits::e4)
	{
		std::cout << "max msaa samples are 4\n";
		return vk::SampleCountFlagBits::e4;
	}
	if (counts & vk::SampleCountFlagBits::e2)
	{
		std::cout << "max msaa samples are 2\n";
		return vk::SampleCountFlagBits::e2;
	}
	std::cout << "max msaa samples are 2\n";
	return vk::SampleCountFlagBits::e1;
}

std::vector<char> readFile(std::string const& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("unable to open file " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

vk::ShaderModule createShaderModule(std::vector<char> const& code, vk::Device device)
{
	vk::ShaderModuleCreateInfo createInfo({}, code.size(),
		reinterpret_cast<uint32_t const*>(code.data()));

	vk::ShaderModule shaderModule = device.createShaderModule(createInfo, nullptr);

	return shaderModule;
}

void VulkanResources::createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
	vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
	vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
	vk::DeviceMemory& imageMemory)
{
	vk::ImageCreateInfo imageInfo({}, vk::ImageType::e2D, format, { width, height, 1 }, mipLevels,
		1, numSamples, tiling, usage, vk::SharingMode::eExclusive, {}, {}, vk::ImageLayout::eUndefined);

	image = device.createImage(imageInfo);

	vk::MemoryRequirements memRequirements;
	memRequirements = device.getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo allocInfo(memRequirements.size,
		findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice));

	imageMemory = device.allocateMemory(allocInfo);

	device.bindImageMemory(image, imageMemory, 0);
}

uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice physicalDevice)
{
	vk::PhysicalDeviceMemoryProperties memProperties;
	memProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("couldn't find suitable memory type");
}

void VulkanResources::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
	vk::ImageLayout newLayout, uint32_t mipLevels)
{
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	vk::ImageSubresourceRange subresourceRange({}, 0, mipLevels, 0, 1);
	vk::ImageMemoryBarrier barrier({}, {}, oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		image, subresourceRange);

	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (hasStencilComponent(format))
		{
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition");
	}

	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);

	endSingleTimeCommands(commandBuffer, device, commandPool, graphicsQueue);
}

vk::CommandBuffer beginSingleTimeCommands(vk::Device device, vk::CommandPool commandPool)
{
	vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);

	vk::CommandBuffer commandBuffer;
	commandBuffer = device.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

bool hasStencilComponent(vk::Format format)
{
	return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

void endSingleTimeCommands(vk::CommandBuffer commandBuffer, vk::Device device, vk::CommandPool commandPool, vk::Queue graphicsQueue)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer);

	graphicsQueue.submit(submitInfo, {});
	graphicsQueue.waitIdle();

	device.freeCommandBuffers(commandPool, commandBuffer);
}

void VulkanResources::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
	vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
	vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eExclusive);

	buffer = device.createBuffer(bufferInfo);

	vk::MemoryRequirements memRequirements;
	memRequirements = device.getBufferMemoryRequirements(buffer);

	vk::MemoryAllocateInfo allocInfo(memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice));

	bufferMemory = device.allocateMemory(allocInfo);

	device.bindBufferMemory(buffer, bufferMemory, 0);
}

void VulkanResources::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	vk::ImageSubresourceLayers subresource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	vk::BufferImageCopy region(0, 0, 0, subresource, { 0, 0, 0 }, { width, height, 1 });

	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

	endSingleTimeCommands(commandBuffer, device, commandPool, graphicsQueue);
}

void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth,
	int32_t texHeight, uint32_t mipLevels, vk::Device device, vk::CommandPool commandPool,
	vk::PhysicalDevice physicalDevice, vk::Queue graphicsQueue)
{
	vk::FormatProperties formatProperties;
	formatProperties = physicalDevice.getFormatProperties(imageFormat);

	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
	{
		throw std::runtime_error("texture image format doesn't support linear blitting");
	}

	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	vk::ImageMemoryBarrier barrier;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;
	for (uint32_t i = 1; i < mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

		vk::ImageSubresourceLayers srcSubresource(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
		vk::ImageSubresourceLayers dstSubresource(vk::ImageAspectFlagBits::eColor, i, 0, 1);
		vk::ImageBlit blit(srcSubresource, {}, dstSubresource, {});
		blit.srcOffsets[0] = vk::Offset3D{ 0,0,0 };
		blit.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
		blit.dstOffsets[0] = vk::Offset3D{ 0,0,0 };
		blit.dstOffsets[1] = vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };

		commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
			vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

	endSingleTimeCommands(commandBuffer, device, commandPool, graphicsQueue);
}

void loadModel(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
	vertices = {
		{{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
		{{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		{{-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}
	};
	indices = { 0, 1, 2, 2, 3, 0 };
	std::cout << "loaded quad vertices and indices\n";
}

void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size,
	vk::Device device, vk::CommandPool commandPool, vk::Queue graphicsQueue)
{
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	vk::BufferCopy copyRegion(0, 0, size);
	commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

	endSingleTimeCommands(commandBuffer, device, commandPool, graphicsQueue);
}