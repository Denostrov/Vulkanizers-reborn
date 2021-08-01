#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <cassert>
#include <any>
#include <fstream>
#include <iostream>
#include <GLFW/glfw3.h>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else 
const bool enableValidationLayers = true;
#endif

enum class Rotations
{
	eNorth, eEast, eSouth, eWest
};

enum class SpriteLayers
{
	eBackground, eGround, eAir, eGUI
};

enum class SettingTypes
{
	eUInt, eUShort, eFloat, eString
};

struct Settings
{
	static unsigned int WINDOW_WIDTH;
	static unsigned int WINDOW_HEIGHT;
	static float CURSOR_SIZE;
	static constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2;
	static constexpr unsigned short MAX_SPRITES = 512;
	static constexpr unsigned short MAX_TEXTURES = 64;
	static std::string SPRITE_FRAG_SHADER_PATH;
	static std::string SPRITE_VERT_SHADER_PATH;
	static std::string FRACTAL_FRAG_SHADER_PATH;
	static std::string FRACTAL_VERT_SHADER_PATH;
};

const std::vector<char const*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<char const*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//keys that are polled every frame
const std::vector<short> mappedKeys = {
	GLFW_KEY_ESCAPE, GLFW_KEY_SPACE, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT,
	GLFW_KEY_RIGHT, GLFW_KEY_K, GLFW_KEY_P, GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_S
};

const std::vector<char> mappedMouseKeys = {
	GLFW_MOUSE_BUTTON_LEFT
};

const std::vector<char const*> configSettingNames = {
	"WINDOW_WIDTH", "WINDOW_HEIGHT", "CURSOR_SIZE", "MAX_FRAMES_IN_FLIGHT", "MAX_SPRITES", "MAX_TEXTURES", "FRAG_SHADER_PATH", "VERT_SHADER_PATH"
};

//returns underlying type of the enumerator
template<typename E>
constexpr auto toUType(E enumerator) noexcept
{
	return static_cast<std::underlying_type_t<E>>(enumerator);
}

//floor value but constexpr
constexpr int cfloor(double num)
{
	return (static_cast<double>(static_cast<int>(num)) == num)
		? static_cast<int>(num)
		: static_cast<int>(num) - ((num > 0) ? 0 : 1);
}

constexpr float convertColumnToX(int column) noexcept
{
	return -1.0f + 0.25f * column + 1.0f / 8.0f;
}

constexpr float convertRowToY(int row) noexcept
{
	return -1.0f + 0.25f * row + 1.0f / 8.0f;
}

constexpr int convertXToColumn(double x) noexcept
{
	return cfloor(x * 4.0 + 4.0);
}

constexpr int convertYToRow(double y) noexcept
{
	return cfloor(y * 4.0 + 4.0);
}

std::any loadSetting(std::ifstream& file, std::string const& settingName, SettingTypes settingType);

void loadConfig(std::string const& filename);