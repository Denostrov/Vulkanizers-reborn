#include "GraphicsComponent.h"

GraphicsComponent::GraphicsComponent()
	:spriteIndex{ -1 }, vulkan{ nullptr }
{

}

GraphicsComponent::GraphicsComponent(VulkanResources* vulkan, float posX, float posY, SpriteLayers layer, float sizeX, float sizeY, float rotation, vk::Sampler sampler, Texture* texture)
	: vulkan{ vulkan }
{
	spriteIndex = vulkan->addSprite(posX, posY, layer, sizeX, sizeY, rotation,
		sampler, texture, this);
}

GraphicsComponent::~GraphicsComponent()
{
	//check if sprite exists and remove it
	if (spriteIndex != -1)
	{
		vulkan->removeSprite(spriteIndex);
	}
}

GraphicsComponent::GraphicsComponent(GraphicsComponent&& other) noexcept
	:spriteIndex{ -1 }, vulkan{ nullptr } //make other destructible after swap
{
	swap(*this, other);
}

GraphicsComponent& GraphicsComponent::operator=(GraphicsComponent other) noexcept
{
	swap(*this, other);

	return *this;
}

void GraphicsComponent::moveSprite(float posX, float posY)
{
	assert(spriteIndex != -1 && "moved nonexistent sprite");

	vulkan->spritesToRender->getSprites()[spriteIndex].movePosition(posX, posY);
}