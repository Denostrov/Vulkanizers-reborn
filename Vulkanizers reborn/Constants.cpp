#include "Constants.h"

unsigned int Settings::WINDOW_WIDTH = 800;
unsigned int Settings::WINDOW_HEIGHT = 800;
float Settings::CURSOR_SIZE = 20.0f;

std::string Settings::FRAG_SHADER_PATH = "shaders/frag.spv";
std::string Settings::VERT_SHADER_PATH = "shaders/vert.spv";

std::any loadSetting(std::ifstream& file, std::string const& settingName, SettingTypes settingType)
{
	std::string name;
	std::any value;
	file >> name;
	if (file.fail())
	{
		throw std::runtime_error("config file input error");
	}
	if (name != settingName)
	{
		throw std::runtime_error(settingName + " doesn't match config name");
	}
	switch (settingType)
	{
	case SettingTypes::eUInt:
	{
		unsigned int temp;
		file >> temp;
		value = temp;
		break;
	}
	case SettingTypes::eUShort:
	{
		unsigned short temp;
		file >> temp;
		value = temp;
		break;
	}
	case SettingTypes::eFloat:
	{
		float temp;
		file >> temp;
		value = temp;
		break;
	}
	case SettingTypes::eString:
	{
		std::string temp;
		file >> temp;
		value = temp;
		break;
	}
	default:
		throw std::runtime_error("unknown type in config loadSetting()");
		break;
	}
	if (file.fail())
	{
		throw std::runtime_error("config file value error");
	}
	return value;
}

void loadConfig(std::string const& filename)
{
	std::ifstream file{ filename, file.binary | file.in };
	if (!file.is_open())
	{
		std::cout << "couldn't open config file\n";
		return;
	}

	Settings::WINDOW_WIDTH = std::any_cast<unsigned int>(loadSetting(file, "WINDOW_WIDTH", SettingTypes::eUInt));
	Settings::WINDOW_HEIGHT = std::any_cast<unsigned int>(loadSetting(file, "WINDOW_HEIGHT", SettingTypes::eUInt));
	Settings::CURSOR_SIZE = std::any_cast<float>(loadSetting(file, "CURSOR_SIZE", SettingTypes::eFloat));
	Settings::FRAG_SHADER_PATH = std::any_cast<std::string>(loadSetting(file, "FRAG_SHADER_PATH", SettingTypes::eString));
	Settings::VERT_SHADER_PATH = std::any_cast<std::string>(loadSetting(file, "VERT_SHADER_PATH", SettingTypes::eString));
}