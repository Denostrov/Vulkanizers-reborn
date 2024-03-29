#pragma once

#include "Constants.h"
#include "VulkanResources.h"
#include "GraphicsComponent.h"

class VulkanResources;
class Piece;

class Cursor
{
public:
	//spriteless cursor
	Cursor();
	//make cursor at 0,0 of size
	Cursor(VulkanResources* vulkan, float size);
	~Cursor() = default;

	Cursor(Cursor const&) = delete;
	Cursor& operator=(Cursor const&) = delete;
	Cursor(Cursor&&) = default;
	Cursor& operator=(Cursor&&) = default;

	void recreateSprite() { createSprite(sprite.vulkan); }	//remake the graphics component
	void update(GLFWwindow* window);						//update position and move sprite
	void createSprite(VulkanResources* vulkan);
	void deleteSprite();
	void disable(GLFWwindow* window);
	void enable(VulkanResources* vulkan);

	double xPos;
	double yPos;
	double size;
	double prevXPos;
	double prevYPos;
	float sensitivity;
private:
	GraphicsComponent sprite;
};

enum class OutlineTypes
{
	eCursorHighlight, eCursorSelect, eMoveHighlight, eCaptureHighlight
};

class Outline
{
public:
	//shell outline
	Outline();

	//make outline at row/column
	Outline(VulkanResources* vulkan, int row, int column, OutlineTypes type);
	~Outline() = default;

	//can only move outlines
	Outline(Outline const&) = delete;
	Outline& operator=(Outline const&) = delete;
	Outline(Outline&&) = default;
	Outline& operator=(Outline&&) = default;

	void recreateSprite() { createSprite(sprite.vulkan); }	//remake the graphics component

	int row;
	int column;
private:
	void createSprite(VulkanResources* vulkan);

	OutlineTypes type;
	GraphicsComponent sprite;
};