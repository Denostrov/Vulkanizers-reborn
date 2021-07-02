#pragma once

#include "Constants.h"
#include <vulkan/vulkan.hpp>

class VulkanResources;

class Texture
{
public:
	//create shell texture
	Texture();

	Texture(std::string const& filename, VulkanResources* vulkan);
	//free texture resources
	~Texture();

	//can move textures but can't copy
	Texture(Texture const&) = delete;
	
	Texture(Texture&& other) noexcept;		
	Texture& operator=(Texture other) noexcept;

	friend void swap(Texture& first, Texture& second) noexcept
	{
		using std::swap;

		swap(first.imageView, second.imageView);
		swap(first.vulkan, second.vulkan);
		swap(first.image, second.image);
		swap(first.imageMemory, second.imageMemory);
	}

	vk::ImageView imageView;
private:

	VulkanResources* vulkan;
	vk::Image image;
	vk::DeviceMemory imageMemory;
};