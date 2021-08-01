#pragma once

#include "Constants.h"
#include "Texture.h"
#include <vulkan/vulkan.hpp>

class VulkanResources;
class Object;
class GraphicsComponent;

class SpritePool
{
	class Sprite
	{
	public:
		Sprite();
		~Sprite() = default;

		//allocate all vulkan resources preemptively
		explicit Sprite(VulkanResources* vulkan);

		//can only move sprites, can't copy
		Sprite(Sprite&&) = default;
		Sprite& operator=(Sprite&&) = default;
		Sprite(Sprite const&) = delete;
		Sprite& operator= (Sprite const&) = delete;

		//create sprite on the gpu
		void instantiate(float posX, float posY, SpriteLayers layer, float sizeX, float sizeY, float rotation,
			VulkanResources* vk, vk::Sampler sampler, Texture* texture, GraphicsComponent* object);
		//free resources
		void destroy(VulkanResources* vk);

		void update(uint32_t imageIndex);
		void movePosition(float newX, float newY);

		unsigned char remainingUpdates;	//how many images in swap chain
		bool isRemoved; //if sprite is marked for deletion
		std::vector<vk::DescriptorSet> descriptorSets;
		//pointer to owner object
		GraphicsComponent* object;

	private:
		std::vector<vk::Buffer> uniformBuffers;
		std::vector<vk::DeviceMemory> uniformBuffersMemory;

		float posX;
		float posY;
		SpriteLayers layer;
		float sizeX;
		float sizeY;
		float rotation;

		void updateUniformBuffers(uint32_t imageIndex);

		std::vector<void*> mapped;
		vk::Sampler textureSampler;
		Texture* texture;
	};

public:
	SpritePool(VulkanResources* vulkan);
	~SpritePool();

	short addSprite(float posX, float posY, SpriteLayers layer, float sizeX, float sizeY, float rotation,
		 vk::Sampler sampler, Texture* texture, GraphicsComponent* object); //return index of sprite in array
	void removeSprite(unsigned short index);	//marks sprite for deletion
	void update(uint32_t imageIndex);

	unsigned short size() const { return spriteCount; }
	auto& getSprites() { return sprites; }
private:
	void deleteSprite(unsigned short index);	//frees the sprites resources
	void clear();								//immediately destroys all sprites

	VulkanResources* vulkan;
	short spriteCount;
	std::array<Sprite, Settings::MAX_SPRITES> sprites;
	std::array<unsigned char, Settings::MAX_SPRITES> frameCounts; //counts frames since sprite was removed
};