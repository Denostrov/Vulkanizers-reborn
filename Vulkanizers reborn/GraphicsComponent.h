#pragma once

#include "VulkanResources.h"

class GraphicsComponent
{
public:
	//creates empty component
	GraphicsComponent();
	//creates a sprite at pos
	GraphicsComponent(VulkanResources* vulkan, float posX, float posY, SpriteLayers layer, float sizeX, float sizeY, float rotation, vk::Sampler sampler, Texture* texture);
	//clears sprite 
	~GraphicsComponent();

	GraphicsComponent(GraphicsComponent const& other) = delete;	//can't copy component

	GraphicsComponent(GraphicsComponent&& other) noexcept;		//can move component
	GraphicsComponent& operator=(GraphicsComponent other) noexcept;

	friend void swap(GraphicsComponent& first, GraphicsComponent& second) noexcept
	{
		using std::swap;

		//two empty graphics components swapped
		if (!first.vulkan && !second.vulkan) return;

		auto& sprites = (first.vulkan) ? first.vulkan->spritesToRender->getSprites() : second.vulkan->spritesToRender->getSprites();
		//first sprite is empty, update second's object location
		if (first.spriteIndex == -1 && second.spriteIndex != -1)
		{
			sprites[second.spriteIndex].object = &first;
		}
		//second sprite is empty, update first's object location
		else if (first.spriteIndex != -1 && second.spriteIndex == -1)
		{
			sprites[first.spriteIndex].object = &second;
		}
		//both sprites exist, swap object locations
		else if (first.spriteIndex != -1 && second.spriteIndex != -1)
		{
			swap(sprites[first.spriteIndex].object, sprites[second.spriteIndex].object);
		}
		swap(first.spriteIndex, second.spriteIndex);
		swap(first.vulkan, second.vulkan);
	}

	void moveSprite(float newX, float newY);

	short spriteIndex;
	VulkanResources* vulkan;
};