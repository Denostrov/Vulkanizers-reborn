#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define NOMINMAX

#include "Constants.h"
#include "Sprite.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <iostream>
#include <map>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <string>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <thread>

class Game;
struct Vertex;
class GraphicsComponent;

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct GraphicsPipelineData
{
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout layout;
	vk::Pipeline pipeline;
};

class ShaderModulePair
{
public:
	ShaderModulePair();
	ShaderModulePair(std::string const& vertFilename, std::string const& fragFilename, vk::Device device);
	~ShaderModulePair();

	ShaderModulePair(ShaderModulePair const&) = delete;
	ShaderModulePair& operator=(ShaderModulePair const&) = delete;

	ShaderModulePair(ShaderModulePair&& rhs) noexcept;
	ShaderModulePair& operator=(ShaderModulePair&& rhs) noexcept;

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

private:
	void createShaderStages();

	vk::ShaderModule vertShaderModule;
	vk::ShaderModule fragShaderModule;
	vk::Device device;
};

struct PipelineCreateData
{
	ShaderModulePair shaderModules;
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	vk::Viewport viewport;
	vk::Rect2D scissor;
	vk::PipelineViewportStateCreateInfo viewportState;
	vk::PipelineRasterizationStateCreateInfo rasterizer;
	vk::PipelineMultisampleStateCreateInfo multisampling;
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	vk::PipelineColorBlendStateCreateInfo colorBlending;
	vk::PipelineDepthStencilStateCreateInfo depthStencil;
};

static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
std::vector<char> readFile(std::string const& filename);

class VulkanResources
{
public:
	VulkanResources(Game* game);
	~VulkanResources();

	void recreateSwapChain();
	void waitUntilDeviceIsIdle() { device.waitIdle(); }
	void drawFrame();

	short addSprite(float posX, float posY, SpriteLayers layer, float sizeX, float sizeY, float rotation,
		vk::Sampler sampler, Texture* texture, GraphicsComponent* object);	//returns index of sprite in pool
	void removeSprite(unsigned short index);

	vk::ImageView createImageView(vk::Image image, vk::Format format,
		vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
	void	createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
		vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	void	createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
		vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
		vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
		vk::DeviceMemory& imageMemory);
	void	transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout, uint32_t mipLevels);
	void	copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

	GLFWwindow* window;
	Game* game;

	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	std::vector<vk::Image> swapChainImages;
	vk::Extent2D swapChainExtent;
	vk::DescriptorPool descriptorPool;
	vk::CommandPool commandPool;
	vk::Queue graphicsQueue;
	std::vector<GraphicsPipelineData> graphicsPipelinesData;

	vk::Sampler textureSampler;
	std::array<Texture, Settings::MAX_TEXTURES> textures;

	std::unique_ptr<SpritePool> spritesToRender;

	size_t currentFrame = 0;

private:
	void initWindow();
	void initVulkan();
	void createDynamicLoader();
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void cleanupSwapChain();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipelines();
	void createCommandPool();
	void createColorResources();
	void createDepthResources();
	void createFramebuffers();
	void createTextures();
	void createTextureSampler();
	void createVertexBuffer();
	void createIndexBuffer();
	void createDescriptorPool();
	void createCommandBuffers();
	void createSyncObjects();

	void createSprites();
	void updateSprites(uint32_t imageIndex);
	void updateCommandBuffer(uint32_t imageIndex);

	vk::DynamicLoader dynamicLoader;
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugMessenger;
	QueueFamilyIndices queueIndices;
	vk::SurfaceKHR surface;
	vk::Queue presentQueue;
	vk::SwapchainKHR swapChain;
	vk::Format swapChainImageFormat;
	std::vector<vk::ImageView> swapChainImageViews;
	vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
	vk::RenderPass renderPass;
	vk::Image colorImage;
	vk::DeviceMemory colorImageMemory;
	vk::ImageView colorImageView;
	vk::Image depthImage;
	vk::DeviceMemory depthImageMemory;
	vk::ImageView depthImageView;
	std::vector<vk::Framebuffer> swapChainFramebuffers;
	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexBufferMemory;
	vk::Buffer indexBuffer;
	vk::DeviceMemory indexBufferMemory;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<vk::CommandBuffer> commandBuffers;
	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	uint32_t mipLevels = 1;
};

struct SwapChainSupportDetails
{
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		vk::VertexInputBindingDescription bindingDescription(0,
			sizeof(Vertex), vk::VertexInputRate::eVertex);

		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 mvp;
};