#pragma once

#include "VulkanResources.h"
#include "GraphicsComponent.h"
#include "Cursor.h"
#include "Camera.h"
#include <random>
#include <array>
#include <cassert>

class SoundEngine;

class Game
{
public:
	//makes the game
	Game();
	//unmakes the game
	~Game();

	Game(Game const&) = delete;
	Game& operator=(Game const&) = delete;
	Game(Game&&) = delete;
	Game& operator=(Game&&) = delete;

	//starts the game loop
	void start();
	//remakes all object sprites
	void recreateSprites();

	//set GLFW window for getting cursor data
	void setWindow(GLFWwindow* win) noexcept { window = win; }

	bool getWindowWasResized() const noexcept { return windowResized; }
	void setWindowWasResized() noexcept { windowResized = true; }

	Camera camera;
	float steps;
	std::vector<float> fractalData;
	float iterations;
	int sceneID;

	bool cursorEnabled;
	double mWheelMovement;
private:
	void calculateDeltaTime();
	//check window resizing, update cursor/keys
	void processInput();
	void drawFrame();
	void update();
	void resetGame();
	void enableCursor();
	void disableCursor();

	void loadScene(int id);

	bool keysPressed[512];
	bool keysHeld[512];
	//update key pressed/held arrays
	void updateKeyStates();

	bool gameOver;

	std::unique_ptr<SoundEngine> soundEngine;

	std::unique_ptr<VulkanResources> vulkan;

	Cursor cursor;

	std::random_device random;
	std::mt19937 gen;

	GLFWwindow* window;
	bool windowResized;

	double lastFrameTime;
	float deltaTime;
	float updateTime;

	int fpsFramesRendered;
	float fpsTimePassed;
};