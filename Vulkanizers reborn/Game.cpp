#include "Game.h"
#include "Sound.h"

Game::Game()
	:lastFrameTime{ 0 }, deltaTime{ 0 }, fpsFramesRendered{ 0 }, fpsTimePassed{ 0.0f },
	updateTime{ 0.0f }, gameOver{ false }, soundEngine{ std::make_unique<SoundEngine>() }, vulkan{ std::make_unique<VulkanResources>(this) },
	cursor{ vulkan.get(), Settings::CURSOR_SIZE }, random{}, gen{ random() }, camera{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f }, steps{ 100.0f }, sphereSize{ 0.15f },
	cursorEnabled{ true }, mWheelMovement{ 0.0 }
{

	//get window pointer from vulkan
	window = vulkan->window;

	disableCursor();

	//reset input buffers
	std::fill_n(keysPressed, 512, false);
	std::fill_n(keysHeld, 512, false);

	soundEngine->loadSound("sounds/thud.wav");
	soundEngine->loadSound("sounds/clang.wav");
	soundEngine->loadMusic("sounds/violin.wav");
	soundEngine->setMusicVolume(0, 10.0f);
}

Game::~Game()
{

}

void Game::start()
{
	calculateDeltaTime();
	soundEngine->startMusic(0);
	soundEngine->loopMusic(0, true);

	while (!glfwWindowShouldClose(window))
	{
		calculateDeltaTime();
		fpsTimePassed += deltaTime;
		updateTime += deltaTime;
		if (fpsTimePassed > 1.0f)
		{
			std::cout << fpsFramesRendered << "\t" << 1.0f / fpsFramesRendered << "\n";
			fpsTimePassed -= 1.0f;
			fpsFramesRendered = 0;
		}

		int timesUpdated = 0;
		while (updateTime >= 0.01f)	//update 100 times a second
		{
			//update key arrays
			processInput();

			if (keysHeld[GLFW_KEY_W])
			{
				camera.position += 0.01f * camera.direction;
			}
			if (keysHeld[GLFW_KEY_S])
			{
				camera.position -= 0.01f * camera.direction;
			}
			if (keysHeld[GLFW_KEY_A])
			{
				camera.position -= 0.01f * camera.right;
			}
			if (keysHeld[GLFW_KEY_D])
			{
				camera.position += 0.01f * camera.right;
			}
			if (keysHeld[GLFW_KEY_LEFT_SHIFT])
			{
				camera.position += 0.01f * camera.up;
			}
			if (keysHeld[GLFW_KEY_LEFT_CONTROL])
			{
				camera.position -= 0.01f * camera.up;
			}
			if (keysHeld[GLFW_KEY_UP])
			{
				steps += 0.01f * 10.0f;
			}
			if (keysHeld[GLFW_KEY_DOWN])
			{
				steps -= 0.01f * 20.0f;
			}
			if (keysHeld[GLFW_KEY_Q])
			{
				sphereSize -= 0.01f * 0.01f;
			}
			if (keysHeld[GLFW_KEY_E])
			{
				sphereSize += 0.01f * 0.01f;
			}
			if (keysPressed[GLFW_KEY_SPACE])
			{
				if (cursorEnabled)
				{
					disableCursor();
				}
				else
				{
					enableCursor();
				}
			}
			if (mWheelMovement != 0.0)
			{
				camera.focalLength += (float)mWheelMovement * 0.05f;
				if (camera.focalLength <= 0.0f)
				{
					camera.focalLength = 0.0f;
				}
				mWheelMovement = 0.0;
			}

			float newYaw = float(cursor.xPos - cursor.prevXPos) * cursor.sensitivity + camera.yaw;
			float newPitch = float(cursor.yPos - cursor.prevYPos) * cursor.sensitivity + camera.pitch;
			if (newPitch > 89.5f)
			{
				newPitch = 89.5f;
			}
			else if (newPitch < -89.5f)
			{
				newPitch = -89.5f;
			}
			if (newYaw > 180.0f)
			{
				newYaw -= 360.0f;
			}
			else if (newYaw < -180.0f)
			{
				newYaw += 360.0f;
			}
			if (!cursorEnabled)
			{
				camera.orient(newPitch, newYaw);
			}

			updateTime -= 0.01f;
			timesUpdated++;
			//cap updates if rendering needs to catch up
			if (timesUpdated > 4)
			{
				updateTime = 0.0f;
				break;
			}
		}

		drawFrame();

		fpsFramesRendered++;
	}

	soundEngine->stopMusic(0);

	vulkan->waitUntilDeviceIsIdle();
}

void Game::processInput()
{
	glfwPollEvents();

	if (windowResized)
	{
		vulkan->recreateSwapChain();
		windowResized = false;
	}

	updateKeyStates();

	//update cursor position
	cursor.update(window);

	//close the program
	if (keysPressed[GLFW_KEY_ESCAPE])
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

void Game::updateKeyStates()
{
	//iterate relevant keys
	for (auto key : mappedKeys)
	{
		//key is being pressed
		if (glfwGetKey(window, key) == GLFW_PRESS && !keysHeld[key])
		{
			//second frame of key being pressed
			if (keysPressed[key])
			{
				keysHeld[key] = true;
				keysPressed[key] = false;
			}
			//first frame of key being pressed
			else
			{
				keysPressed[key] = true;
			}
		}
		//key is released
		else if (glfwGetKey(window, key) == GLFW_RELEASE)
		{
			keysHeld[key] = false;
			keysPressed[key] = false;
		}
	}
	//iterate mouse keys
	for (auto key : mappedMouseKeys)
	{
		//key is being pressed
		if (glfwGetMouseButton(window, key) == GLFW_PRESS && !keysHeld[key])
		{
			//second frame of key being pressed
			if (keysPressed[key])
			{
				keysHeld[key] = true;
				keysPressed[key] = false;
			}
			//first frame of key being pressed
			else
			{
				keysPressed[key] = true;
			}
		}
		//key is released
		else if (glfwGetMouseButton(window, key) == GLFW_RELEASE)
		{
			keysHeld[key] = false;
			keysPressed[key] = false;
		}
	}
}

void Game::calculateDeltaTime()
{
	double currentTime = glfwGetTime();
	deltaTime = (float)(currentTime - lastFrameTime);
	lastFrameTime = currentTime;
}

void Game::update()
{

}

void Game::drawFrame()
{
	vulkan->drawFrame();
}

void Game::resetGame()
{
	soundEngine->stopMusic(0);
	soundEngine->startMusic(0);
	gameOver = false;
}

void Game::enableCursor()
{
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	cursor.enable(vulkan.get());
	cursorEnabled = true;
}

void Game::disableCursor()
{
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	cursor.disable(window);
	cursorEnabled = false;
}

//make new sprites after pool was cleared
void Game::recreateSprites()
{
	cursor.recreateSprite();
}