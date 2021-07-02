#include "Texture.h"
#include "VulkanResources.h"

Texture::Texture()
	:vulkan{ nullptr }
{

}

Texture::Texture(std::string const& filename, VulkanResources* vulkan)
	:vulkan{ vulkan }
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	vk::DeviceSize imageSize = (uint32_t)texWidth * (uint32_t)texHeight * 4;
	
	if (!pixels)
	{
		throw std::runtime_error("couldn't load texture image " + filename);
	}

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;

	vulkan->createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		stagingBuffer, stagingBufferMemory);

	void* data = vulkan->device.mapMemory(stagingBufferMemory, 0, imageSize);
	memcpy(data, pixels, (size_t)imageSize);
	vulkan->device.unmapMemory(stagingBufferMemory);

	stbi_image_free(pixels);

	vulkan->createImage(texWidth, texHeight, 1, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Unorm,
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, image, imageMemory);

	vulkan->transitionImageLayout(image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal, 1);

	vulkan->copyBufferToImage(stagingBuffer, image, (uint32_t)(texWidth), (uint32_t)(texHeight));

	vulkan->transitionImageLayout(image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal, 1);

	vulkan->device.destroyBuffer(stagingBuffer);
	vulkan->device.freeMemory(stagingBufferMemory);

	imageView = vulkan->createImageView(image, vk::Format::eR8G8B8A8Unorm,
		vk::ImageAspectFlagBits::eColor, 1);
}

//swap other with dummy shell
Texture::Texture(Texture&& other) noexcept
	:Texture{}
{
	swap(*this, other);
}

//move construct other and delete it after swapping
Texture& Texture::operator=(Texture other) noexcept
{
	swap(*this, other);

	return *this;
}

Texture::~Texture()
{
	if (vulkan)
	{
		vulkan->device.destroyImageView(imageView);
		vulkan->device.destroyImage(image);
		vulkan->device.freeMemory(imageMemory);
	}
}