#include "Sound.h"
#include <thread>

SoundEngine::SoundEngine()
{

}

SoundEngine::~SoundEngine()
{
	for (auto sound : sounds)
	{
		sound->stop();
		delete sound;
	}
	for (auto buffer : soundBuffers)
	{
		delete buffer;
	}
	for (auto melody : music)
	{
		melody->stop();
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	for (auto melody : music)
	{
		delete melody;
	}
}

void SoundEngine::loadSound(std::string const& filename)
{
	sf::SoundBuffer* buffer = new sf::SoundBuffer();
	if (!(buffer->loadFromFile(filename)))
	{
		throw std::runtime_error("couldn't load file " + filename);
	}
	soundBuffers.push_back(buffer);
	sf::Sound* sound = new sf::Sound();
	sound->setBuffer(*buffer);
	sounds.push_back(sound);
}

void SoundEngine::loadMusic(std::string const& filename)
{
	sf::Music* melody = new sf::Music();
	if (!melody->openFromFile(filename))
	{
		throw std::runtime_error("couldn't load file " + filename);
	}
	music.push_back(melody);
}

void SoundEngine::playSound(int index)
{
	sounds[index]->play();
}

void SoundEngine::startMusic(int index)
{
	music[index]->play();
}

void SoundEngine::stopMusic(int index)
{
	music[index]->stop();
}

void SoundEngine::loopMusic(int index, bool b)
{
	music[index]->setLoop(b);
}

void SoundEngine::setSoundVolume(int index, float volume)
{
	sounds[index]->setVolume(volume);
}

void SoundEngine::setMusicVolume(int index, float volume)
{
	music[index]->setVolume(volume);
}