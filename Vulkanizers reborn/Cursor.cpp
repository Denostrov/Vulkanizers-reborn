#include "Cursor.h"

Cursor::Cursor()
	:xPos{ 0.0 }, yPos{ 0.0 }, size{ 0.0 }, prevXPos{ 0.0 }, prevYPos{ 0.0 }, sensitivity{ 0.05f }
{

}

Cursor::Cursor(VulkanResources* vulkan, float size)
	: xPos{ 0.0 }, yPos{ 0.0 }, size{ size }, prevXPos{ 0.0 }, prevYPos{ 0.0 }, sensitivity{ 0.05f }
{
	assert(vulkan && "created cursor with nullptr vulkan");
	createSprite(vulkan);
}

void Cursor::createSprite(VulkanResources* vulkan)
{
	sprite = GraphicsComponent{ vulkan, (float)(xPos + size / 800.0), (float)(yPos - size / 800.0), SpriteLayers::eGUI, (float)(size / 800.0), (float)(size / 800.0), 0.0f, vulkan->textureSampler, &vulkan->textures[13] };
}

void Cursor::update(GLFWwindow* window)
{
	prevXPos = xPos;
	prevYPos = yPos;

	//poll cursor position
	double newXPos = 0.0, newYPos = 0.0;
	glfwGetCursorPos(window, &newXPos, &newYPos);

	//convert screen coords to (-1, 1) range
	if (sprite.vulkan)
	{
		newXPos = newXPos * 2.0 / sprite.vulkan->swapChainExtent.width - 1.0;
		newYPos = -newYPos * 2.0 / sprite.vulkan->swapChainExtent.height + 1.0;
	}

	//move sprite so top left part of cursor is at the center
	if (newXPos != xPos || newYPos != yPos)
	{
		xPos = newXPos;
		yPos = newYPos;
		if (sprite.vulkan)
		{
			sprite.moveSprite((float)(xPos + size / 800.0), (float)(yPos - size / 800.0));
		}
	}
}

Outline::Outline()
	:row{ -1 }, column{ -1 }
{

}

Outline::Outline(VulkanResources* vulkan, int row, int column, OutlineTypes type)
	: row{ row }, column{ column }, type{ type }
{
	createSprite(vulkan);
}

void Outline::createSprite(VulkanResources* vulkan)
{
	assert(row != -1 && column != -1 && "created sprite for empty outline");

	int textureIndex = -1;
	switch (type)
	{
	case OutlineTypes::eCursorHighlight:
		textureIndex = 14;
		break;
	case OutlineTypes::eCursorSelect:
		textureIndex = 15;
		break;
	case OutlineTypes::eMoveHighlight:
		textureIndex = 16;
		break;
	case OutlineTypes::eCaptureHighlight:
		textureIndex = 17;
		break;
	default:
		break;
	}

	assert(textureIndex != -1 && "unknown outline type created");
	sprite = GraphicsComponent{ vulkan, convertColumnToX(column), convertRowToY(row), SpriteLayers::eAir, 100.0f / 800.0f, 100.0f / 800.0f, 0.0f, vulkan->textureSampler, &vulkan->textures[textureIndex] };
}