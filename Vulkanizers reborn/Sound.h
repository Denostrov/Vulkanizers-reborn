#pragma once

#include <SFML/Audio.hpp>

class SoundEngine
{
public:
	SoundEngine();
	~SoundEngine();

	void loadSound(std::string const& filename);
	void loadMusic(std::string const& filename);
	void playSound(int index);
	void startMusic(int index);
	void stopMusic(int index);
	void loopMusic(int index, bool b);
	void setSoundVolume(int index, float volume);
	void setMusicVolume(int index, float volume);

	std::vector<sf::Sound*> sounds;
	std::vector<sf::SoundBuffer*> soundBuffers;
	std::vector<sf::Music*> music;
};