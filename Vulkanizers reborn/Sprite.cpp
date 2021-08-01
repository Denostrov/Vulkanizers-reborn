#include "Sprite.h"
#include "VulkanResources.h"
#include "Game.h"

SpritePool::Sprite::Sprite()
	:remainingUpdates{ 0 }, isRemoved{ true }, object{ nullptr }, posX{ 0.0f }, posY{ 0.0f }, layer{ SpriteLayers::eGUI }, sizeX{ -1.0f }, sizeY{ -1.0f }, rotation{ 0.0f }, texture{ nullptr }
{}

SpritePool::Sprite::Sprite(VulkanResources* vulkan)
	:object{ nullptr }, texture{ nullptr }
{
	uniformBuffers.resize(vulkan->swapChainImages.size());
	uniformBuffersMemory.resize(vulkan->swapChainImages.size());
	mapped.resize(vulkan->swapChainImages.size());

	for (size_t i = 0; i < vulkan->swapChainImages.size(); i++)
	{
		vulkan->createBuffer(sizeof(UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			uniformBuffers[i], uniformBuffersMemory[i]);
		mapped[i] = vulkan->device.mapMemory(uniformBuffersMemory[i], 0, sizeof(UniformBufferObject));
	}

	std::vector<vk::DescriptorSetLayout> layouts(vulkan->swapChainImages.size(), vulkan->graphicsPipelinesData[0].descriptorSetLayout);
	vk::DescriptorSetAllocateInfo allocInfo(vulkan->descriptorPool, (uint32_t)vulkan->swapChainImages.size(), layouts.data());

	descriptorSets = vulkan->device.allocateDescriptorSets(allocInfo);
}

void SpritePool::Sprite::instantiate(float posX, float posY, SpriteLayers layer, float sizeX, float sizeY, float rotation,
	VulkanResources* vk, vk::Sampler sampler, Texture* texture, GraphicsComponent* object)
{
	this->posX = posX;
	this->posY = posY;
	this->layer = layer;
	this->sizeX = sizeX;
	this->sizeY = sizeY;
	this->rotation = rotation;
	this->textureSampler = sampler;
	this->texture = texture;
	this->object = object;
	this->isRemoved = false;
	this->remainingUpdates = 0;

	for (size_t i = 0; i < vk->swapChainImages.size(); i++)
	{
		UniformBufferObject ubo = {};
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(posX, posY, toUType(layer) / 10.0f));
		model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(sizeX, sizeY, 1.0f));
		ubo.mvp = glm::ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f) * model;
		memcpy(mapped[i], &ubo, sizeof(ubo));
	}

	for (size_t i = 0; i < vk->swapChainImages.size(); i++)
	{
		vk::DescriptorBufferInfo bufferInfo(uniformBuffers[i], 0, sizeof(UniformBufferObject));

		vk::DescriptorImageInfo imageInfo(textureSampler, texture->imageView, vk::ImageLayout::eShaderReadOnlyOptimal);

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {};
		descriptorWrites[0] = vk::WriteDescriptorSet(descriptorSets[i], 0, 0, 1,
			vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo, nullptr);
		descriptorWrites[1] = vk::WriteDescriptorSet(descriptorSets[i], 1, 0, 1,
			vk::DescriptorType::eCombinedImageSampler, &imageInfo, nullptr, nullptr);

		vk->device.updateDescriptorSets(descriptorWrites, nullptr);
	}
}

//free sprite resources
void SpritePool::Sprite::destroy(VulkanResources* vulkan)
{
	for (size_t i = 0; i < vulkan->swapChainImages.size(); i++)
	{
		vulkan->device.destroyBuffer(uniformBuffers[i]);
		vulkan->device.freeMemory(uniformBuffersMemory[i]);
	}
	vulkan->device.freeDescriptorSets(vulkan->descriptorPool, descriptorSets);
}

void SpritePool::Sprite::update(uint32_t imageIndex)
{
	updateUniformBuffers(imageIndex);
}

void SpritePool::Sprite::movePosition(float newX, float newY)
{
	posX = newX;
	posY = newY;
	remainingUpdates = (unsigned char)uniformBuffers.size();
}

void SpritePool::Sprite::updateUniformBuffers(uint32_t imageIndex)
{
	remainingUpdates--;
	UniformBufferObject ubo = {};
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(posX, posY, toUType(layer) / 10.0f));
	model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(sizeX, sizeY, 1.0f));
	ubo.mvp = glm::ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f) * model;
	memcpy(mapped[imageIndex], &ubo, sizeof(ubo));
}

SpritePool::SpritePool(VulkanResources* vulkan)
	:spriteCount{ 0 }, vulkan{ vulkan }
{
	//allocate vulkan buffers preemptively
	for (auto& sprite : sprites)
	{
		sprite = Sprite(vulkan);
	}

}

SpritePool::~SpritePool()
{
	clear();
}

short SpritePool::addSprite(float posX, float posY, SpriteLayers layer, float sizeX, float sizeY, float rotation,
	vk::Sampler sampler, Texture* texture, GraphicsComponent* object)
{
	if (spriteCount == Settings::MAX_SPRITES)
	{
		throw std::exception("exceeded sprite limit");
	}

	sprites[spriteCount].instantiate(posX, posY, layer, sizeX, sizeY, rotation, vulkan, sampler, texture, object);
	std::cout << "instantiated sprite at " << spriteCount << " index\n";
	spriteCount++;
	return spriteCount - 1;
}

//mark sprite for removal and detach graphics component
void SpritePool::removeSprite(unsigned short index)
{
	if (index >= spriteCount)
	{
		throw std::exception("tried removing already deleted sprite");
	}
	sprites[index].isRemoved = true;
	sprites[index].object = nullptr;
	std::cout << "removed sprite at " << index << " index\n";
}

//swap removed sprite with last valid sprite and free resources
void SpritePool::deleteSprite(unsigned short index)
{
	std::swap(sprites[spriteCount - 1], sprites[index]);
	std::swap(frameCounts[spriteCount - 1], frameCounts[index]);
	spriteCount--;
	std::cout << "swapped sprites at indices " << spriteCount << " and " << index << "\n";
	//update the moved sprite's object
	if (!sprites[index].isRemoved)
	{
		sprites[index].object->spriteIndex = index;
	}

	frameCounts[spriteCount] = 0;
}

void SpritePool::clear()
{
	//wait until gpu is done
	vulkan->device.waitIdle();
	for (int i = 0; i < sprites.size(); i++)
	{
		//make objects forget about their sprites
		if (sprites[i].object)
		{
			sprites[i].object->spriteIndex = -1;
		}
		sprites[i].destroy(vulkan);
	}
	std::cout << "destroyed all sprites\n";
	spriteCount = 0;
}

void SpritePool::update(uint32_t imageIndex)
{
	for (int i = 0; i < spriteCount; i++)
	{
		if (sprites[i].isRemoved)
		{
			frameCounts[i]++;
			if (frameCounts[i] == vulkan->swapChainImages.size())
			{
				deleteSprite(i);
				i--;
			}
		}
		else if (sprites[i].remainingUpdates > 0)
		{
			sprites[i].update(imageIndex);
		}
	}
}