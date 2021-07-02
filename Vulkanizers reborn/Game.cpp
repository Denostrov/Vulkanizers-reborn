#include "Game.h"
#include "Sound.h"

Game::Game()
	:lastFrameTime{ 0 }, deltaTime{ 0 }, fpsFramesRendered{ 0 }, fpsTimePassed{ 0.0f },
	updateTime{ 0.0f }, gameOver{ false }, soundEngine{ std::make_unique<SoundEngine>() }, vulkan{ std::make_unique<VulkanResources>(this) },
	cursor{ vulkan.get(), DEFAULT_CURSOR_SIZE }, whitesMove{ true }, pieceSelected{ false }, random{}, gen{ random() }
{

	//get window pointer from vulkan
	window = vulkan->window;

	//reset input buffers
	std::fill_n(keysPressed, 512, false);
	std::fill_n(keysHeld, 512, false);

	area = new PlayArea(vulkan.get());

	//allocate space for highlights
	highlights.reserve(32);

	soundEngine->loadSound("sounds/thud.wav");
	soundEngine->loadSound("sounds/clang.wav");
	soundEngine->loadMusic("sounds/violin.wav");
	soundEngine->setMusicVolume(0, 10.0f);
}

bool Game::previousFrameTurnWasWhite = false;

Game::~Game()
{
	delete area;
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

			if (keysPressed[GLFW_KEY_SPACE])
			{
				vulkan->recreateSwapChain();
			}
			if (keysPressed[GLFW_MOUSE_BUTTON_LEFT] && whitesMove)
			{
				int cursorTile = convertXToColumn(cursor.xPos) + 8 * convertYToRow(cursor.yPos);
				if (!pieceSelected)
				{
				selectingPiece:

					if ((cursorTile >= 0 && cursorTile <= 63) && area->board[cursorTile] &&
						((whitesMove && area->board[cursorTile]->getColor() == TeamColors::eWhite) || (!whitesMove && area->board[cursorTile]->getColor() == TeamColors::eBlack)))
					{
						selectPiece(area->board[cursorTile], cursorTile);
					}
					else
					{
						selectPiece(nullptr, -1);
					}
				}
				else
				{
					bool movedPiece = false;
					for (auto& tile : highlights)
					{
						if (tile.row == (cursorTile / 8) && tile.column == (cursorTile % 8))
						{
							movePiece(cursor.selectedPiece->getColumn() + 8 * cursor.selectedPiece->getRow(), cursorTile);
							movedPiece = true;
							selectPiece(nullptr, -1);
							whitesMove = !whitesMove;
							break;
						}
					}
					if (!movedPiece)
					{
						goto selectingPiece;
					}
				}
			}
			if (!whitesMove)
			{
				moveAI();
			}
			if (!gameOver)
			{
				update();
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
	setCursorOutline();
	if (previousFrameTurnWasWhite != whitesMove)
	{
		selectPiece(nullptr, -1);
		previousFrameTurnWasWhite = whitesMove;
	}
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
	delete area;
	area = new PlayArea(vulkan.get());
}

//make new sprites after pool was cleared
void Game::recreateSprites()
{
	cursor.recreateSprite();
	setCursorOutline();
	for (auto& highlight : highlights)
	{
		highlight.recreateSprite();
	}
	if (area)
	{
		area->recreateSprite();
		for (auto& piece : area->board)
		{
			if (piece)
			{
				piece->recreateSprite();
			}
		}
	}
}

void Game::selectPiece(Piece* piece, int location)
{
	highlights.clear();

	cursor.selectedPiece = piece;

	if (piece)
	{
		pieceSelected = true;
		std::cout << "selected a piece\n";
	}
	else
	{
		pieceSelected = false;
		std::cout << "selected nothing\n";
		return;
	}

	//get tiles that piece can move to
	std::vector<int> legalTiles = getLegalMoveTiles(piece->getType(), piece->getColor(), location);
	for (auto tile : legalTiles)
	{
		highlights.push_back(Outline(vulkan.get(), tile / 8, tile % 8, OutlineTypes::eMoveHighlight));
	}

	legalTiles = getLegalCaptureTiles(piece->getType(), piece->getColor(), location);
	for (auto tile : legalTiles)
	{
		highlights.push_back(Outline(vulkan.get(), tile / 8, tile % 8, OutlineTypes::eCaptureHighlight));
	}
}

std::vector<int> Game::getLegalMoveTiles(PieceTypes type, TeamColors color, int location)
{
	std::vector<int> result;
	switch (type)
	{
	case PieceTypes::ePawn:
		switch (color)
		{
		case TeamColors::eBlack:
			if ((location - 8) >= 0 && !area->board[location - 8])
			{
				result.push_back(location - 8);
			}
			if ((location - 16) >= 0 && !area->board[location]->alreadyMoved && !area->board[location - 16])
			{
				result.push_back(location - 16);
			}
			break;
		case TeamColors::eWhite:
			if ((location + 8) <= 63 && !area->board[location + 8])
			{
				result.push_back(location + 8);
			}
			if ((location + 16) <= 63 && !area->board[location]->alreadyMoved && !area->board[location + 16])
			{
				result.push_back(location + 16);
			}
			break;
		default:
			break;
		}
		break;
	case PieceTypes::eRook:
	{
		int distance = 1;
		while ((location % 8) + distance <= 7 && !area->board[location + distance])
		{
			result.push_back(location + distance);
			distance++;
		}
		distance = 1;
		while ((location % 8) - distance >= 0 && !area->board[location - distance])
		{
			result.push_back(location - distance);
			distance++;
		}
		distance = 1;
		while ((location + 8 * distance) <= 63 && !area->board[location + 8 * distance])
		{
			result.push_back(location + 8 * distance);
			distance++;
		}
		distance = 1;
		while ((location - 8 * distance) >= 0 && !area->board[location - 8 * distance])
		{
			result.push_back(location - 8 * distance);
			distance++;
		}
		break;
	}
	case PieceTypes::eKnight:
		if (((location % 8) > 0 && location + 15 <= 63) && !area->board[location + 15])
		{
			result.push_back(location + 15);
		}
		if (((location % 8) < 7 && location + 17 <= 63) && !area->board[location + 17])
		{
			result.push_back(location + 17);
		}
		if (((location % 8) > 1 && location + 6 <= 63) && !area->board[location + 6])
		{
			result.push_back(location + 6);
		}
		if (((location % 8) < 6 && location + 10 <= 63) && !area->board[location + 10])
		{
			result.push_back(location + 10);
		}
		if (((location % 8) < 7 && location - 15 >= 0) && !area->board[location - 15])
		{
			result.push_back(location - 15);
		}
		if (((location % 8) > 0 && location - 17 >= 0) && !area->board[location - 17])
		{
			result.push_back(location - 17);
		}
		if (((location % 8) < 6 && location - 6 >= 0) && !area->board[location - 6])
		{
			result.push_back(location - 6);
		}
		if (((location % 8) > 1 && location - 10 >= 0) && !area->board[location - 10])
		{
			result.push_back(location - 10);
		}
		break;
	case PieceTypes::eBishop:
	{
		int distance = 1;
		while ((location % 8) + distance <= 7 && (location + 9 * distance <= 63) && !area->board[location + 9 * distance])
		{
			result.push_back(location + 9 * distance);
			distance++;
		}
		distance = 1;
		while ((location % 8) - distance >= 0 && (location + 7 * distance <= 63) && !area->board[location + 7 * distance])
		{
			result.push_back(location + 7 * distance);
			distance++;
		}
		distance = 1;
		while ((location % 8) + distance <= 7 && (location - 7 * distance >= 0) && !area->board[location - 7 * distance])
		{
			result.push_back(location - 7 * distance);
			distance++;
		}
		distance = 1;
		while ((location % 8) - distance >= 0 && (location - 9 * distance >= 0) && !area->board[location - 9 * distance])
		{
			result.push_back(location - 9 * distance);
			distance++;
		}
		break;
	}
	break;
	case PieceTypes::eQueen:
	{
		result = getLegalMoveTiles(PieceTypes::eRook, color, location);
		auto additionalVec = getLegalMoveTiles(PieceTypes::eBishop, color, location);
		result.insert(result.end(), additionalVec.begin(), additionalVec.end());
		break;
	}
	case PieceTypes::eKing:
		if (location % 8 < 7 && !area->board[location + 1])
		{
			result.push_back(location + 1);
		}
		if (location % 8 < 7 && location + 9 <= 63 && !area->board[location + 9])
		{
			result.push_back(location + 9);
		}
		if (location % 8 < 7 && location - 7 >= 0 && !area->board[location - 7])
		{
			result.push_back(location - 7);
		}
		if (location + 8 <= 63 && !area->board[location + 8])
		{
			result.push_back(location + 8);
		}
		if (location - 8 >= 0 && !area->board[location - 8])
		{
			result.push_back(location - 8);
		}
		if (location % 8 > 0 && !area->board[location - 1])
		{
			result.push_back(location - 1);
		}
		if (location % 8 > 0 && location + 7 <= 63 && !area->board[location + 7])
		{
			result.push_back(location + 7);
		}
		if (location % 8 > 0 && location - 9 >= 0 && !area->board[location - 9])
		{
			result.push_back(location - 9);
		}
		break;
	default:
		break;
	}
	return result;
}

std::vector<int> Game::getLegalCaptureTiles(PieceTypes type, TeamColors color, int location)
{
	std::vector<int> result;
	switch (type)
	{
	case PieceTypes::ePawn:
		switch (color)
		{
		case TeamColors::eBlack:
			if (((location % 8) < 7 && (location - 7) >= 0) && area->board[location - 7] && area->board[location - 7]->getColor() != TeamColors::eBlack)
			{
				result.push_back(location - 7);
			}
			if (((location % 8) > 0 && (location - 9) >= 0) && area->board[location - 9] && area->board[location - 9]->getColor() != TeamColors::eBlack)
			{
				result.push_back(location - 9);
			}
			break;
		case TeamColors::eWhite:
			if (((location % 8) > 0 && (location + 7) <= 63) && area->board[location + 7] && area->board[location + 7]->getColor() != TeamColors::eWhite)
			{
				result.push_back(location + 7);
			}
			if (((location % 8) < 7 && (location + 9) <= 63) && area->board[location + 9] && area->board[location + 9]->getColor() != TeamColors::eWhite)
			{
				result.push_back(location + 9);
			}
			break;
		default:
			break;
		}
		break;
	case PieceTypes::eRook:
	{
		int distance = 1;
		while ((location % 8) + distance <= 7 && !area->board[location + distance])
		{
			distance++;
		}
		if ((location % 8) + distance <= 7 && area->board[location + distance] && area->board[location + distance]->getColor() != color)
		{
			result.push_back(location + distance);
		}
		distance = 1;
		while ((location % 8) - distance >= 0 && !area->board[location - distance])
		{
			distance++;
		}
		if ((location % 8) - distance >= 0 && area->board[location - distance] && area->board[location - distance]->getColor() != color)
		{
			result.push_back(location - distance);
		}
		distance = 1;
		while ((location + 8 * distance) <= 63 && !area->board[location + 8 * distance])
		{
			distance++;
		}
		if ((location + 8 * distance) <= 63 && area->board[location + 8 * distance] && area->board[location + 8 * distance]->getColor() != color)
		{
			result.push_back(location + 8 * distance);
		}
		distance = 1;
		while ((location - 8 * distance) >= 0 && !area->board[location - 8 * distance])
		{
			distance++;
		}
		if ((location - 8 * distance) >= 0 && area->board[location - 8 * distance] && area->board[location - 8 * distance]->getColor() != color)
		{
			result.push_back(location - 8 * distance);
		}
		break;
	}
	case PieceTypes::eKnight:
		if (((location % 8) > 0 && location + 15 <= 63) && area->board[location + 15] && area->board[location + 15]->getColor() != color)
		{
			result.push_back(location + 15);
		}
		if (((location % 8) < 7 && location + 17 <= 63) && area->board[location + 17] && area->board[location + 17]->getColor() != color)
		{
			result.push_back(location + 17);
		}
		if (((location % 8) > 1 && location + 6 <= 63) && area->board[location + 6] && area->board[location + 6]->getColor() != color)
		{
			result.push_back(location + 6);
		}
		if (((location % 8) < 6 && location + 10 <= 63) && area->board[location + 10] && area->board[location + 10]->getColor() != color)
		{
			result.push_back(location + 10);
		}
		if (((location % 8) < 7 && location - 15 >= 0) && area->board[location - 15] && area->board[location - 15]->getColor() != color)
		{
			result.push_back(location - 15);
		}
		if (((location % 8) > 0 && location - 17 >= 0) && area->board[location - 17] && area->board[location - 17]->getColor() != color)
		{
			result.push_back(location - 17);
		}
		if (((location % 8) < 6 && location - 6 >= 0) && area->board[location - 6] && area->board[location - 6]->getColor() != color)
		{
			result.push_back(location - 6);
		}
		if (((location % 8) > 1 && location - 10 >= 0) && area->board[location - 10] && area->board[location - 10]->getColor() != color)
		{
			result.push_back(location - 10);
		}
		break;
	case PieceTypes::eBishop:
	{
		int distance = 1;
		while ((location % 8) + distance <= 7 && (location + 9 * distance <= 63) && !area->board[location + 9 * distance])
		{
			distance++;
		}
		if ((location % 8) + distance <= 7 && (location + 9 * distance <= 63) && area->board[location + 9 * distance] && area->board[location + 9 * distance]->getColor() != color)
		{
			result.push_back(location + 9 * distance);
		}
		distance = 1;
		while ((location % 8) - distance >= 0 && (location + 7 * distance <= 63) && !area->board[location + 7 * distance])
		{
			distance++;
		}
		if ((location % 8) - distance >= 0 && (location + 7 * distance <= 63) && area->board[location + 7 * distance] && area->board[location + 7 * distance]->getColor() != color)
		{
			result.push_back(location + 7 * distance);
		}
		distance = 1;
		while ((location % 8) + distance <= 7 && (location - 7 * distance >= 0) && !area->board[location - 7 * distance])
		{
			distance++;
		}
		if ((location % 8) + distance <= 7 && (location - 7 * distance >= 0) && area->board[location - 7 * distance] && area->board[location - 7 * distance]->getColor() != color)
		{
			result.push_back(location - 7 * distance);
		}
		distance = 1;
		while ((location % 8) - distance >= 0 && (location - 9 * distance >= 0) && !area->board[location - 9 * distance])
		{
			distance++;
		}
		if ((location % 8) - distance >= 0 && (location - 9 * distance >= 0) && area->board[location - 9 * distance] && area->board[location - 9 * distance]->getColor() != color)
		{
			result.push_back(location - 9 * distance);
		}
		break;
	}
	case PieceTypes::eQueen:
	{
		result = getLegalCaptureTiles(PieceTypes::eRook, color, location);
		auto additionalVec = getLegalCaptureTiles(PieceTypes::eBishop, color, location);
		result.insert(result.end(), additionalVec.begin(), additionalVec.end());
		break;
	}
	case PieceTypes::eKing:
		if (location % 8 < 7 && area->board[location + 1] && area->board[location + 1]->getColor() != color)
		{
			result.push_back(location + 1);
		}
		if (location % 8 < 7 && location + 9 <= 63 && area->board[location + 9] && area->board[location + 9]->getColor() != color)
		{
			result.push_back(location + 9);
		}
		if (location % 8 < 7 && location - 7 >= 0 && area->board[location - 7] && area->board[location - 7]->getColor() != color)
		{
			result.push_back(location - 7);
		}
		if (location + 8 <= 63 && area->board[location + 8] && area->board[location + 8]->getColor() != color)
		{
			result.push_back(location + 8);
		}
		if (location - 8 >= 0 && area->board[location - 8] && area->board[location - 8]->getColor() != color)
		{
			result.push_back(location - 8);
		}
		if (location % 8 > 0 && area->board[location - 1] && area->board[location - 1]->getColor() != color)
		{
			result.push_back(location - 1);
		}
		if (location % 8 > 0 && location + 7 <= 63 && area->board[location + 7] && area->board[location + 7]->getColor() != color)
		{
			result.push_back(location + 7);
		}
		if (location % 8 > 0 && location - 9 >= 0 && area->board[location - 9] && area->board[location - 9]->getColor() != color)
		{
			result.push_back(location - 9);
		}
		break;
	default:
		break;
	}
	return result;
}

void Game::movePiece(int from, int to)
{
	assert(area->board[from] && "moved nonexistent piece");

	std::swap(area->board[from], area->board[to]);

	if (area->board[from])
	{
		area->removePiece(from);
		soundEngine->playSound(1);
	}
	else
	{
		soundEngine->playSound(0);
	}

	area->board[to]->move(to / 8, to % 8);
	area->board[to]->alreadyMoved = true;
}

bool Game::moveAI()
{
	std::array<Piece*, 64> boardCopy = area->board;

	int maxValue = -100;
	std::vector<std::pair<int, int>> goodMoves;
	std::vector<int> legalMoves;
	std::vector<int> legalMoves2;
	std::unordered_set<int> whiteControlled;

	int curValue = maxValue;
	for (auto piece2 : boardCopy)
	{
		if (piece2 && piece2->getColor() == TeamColors::eWhite)
		{
			legalMoves2 = getLegalCaptureTiles(piece2->getType(), piece2->getColor(), piece2->getColumn() + 8 * piece2->getRow());
			for (auto index2 : legalMoves2)
			{
				whiteControlled.insert(index2);
			}
		}
	}
	for (auto piece : boardCopy)
	{
		if (piece && piece->getColor() == TeamColors::eBlack)
		{
			legalMoves = getLegalMoveTiles(piece->getType(), piece->getColor(), piece->getColumn() + 8 * piece->getRow());
			for (auto index : legalMoves)
			{
				if (whiteControlled.find(index) == whiteControlled.end() || goodMoves.empty())
				{
					goodMoves.push_back({ piece->getColumn() + 8 * piece->getRow(), index });
				}
			}
		}
	}

	for (auto piece : boardCopy)
	{
		if (piece && piece->getColor() == TeamColors::eBlack)
		{
			legalMoves = getLegalCaptureTiles(piece->getType(), piece->getColor(), piece->getColumn() + 8 * piece->getRow());
			for (auto index : legalMoves)
			{
				goodMoves.push_back({ piece->getColumn() + 8 * piece->getRow(), index });
			}
		}
	}

	std::shuffle(goodMoves.begin(), goodMoves.end(), gen);
	movePiece(goodMoves[goodMoves.size() - 1].first, goodMoves[goodMoves.size() - 1].second);
	whitesMove = !whitesMove;
	return true;
}

//create a cursor outline if aiming at legal piece
void Game::setCursorOutline()
{
	//cursor's row and column
	int row = convertYToRow(cursor.yPos);
	int column = convertXToColumn(cursor.xPos);

	bool diffTileOrTurnChanged = column != cursorOutline.column || row != cursorOutline.row || previousFrameTurnWasWhite != whitesMove;

	//check if cursor is inbounds
	if (row >= 0 && row <= 7 && column >= 0 && column <= 7)
	{
		//check if tile cursor is pointing to changed from outlined or changed who moves
		if (diffTileOrTurnChanged)
		{
			//check if there is a piece under cursor
			if (area->board[column + row * 8])
			{
				//check if piece is eligible to move
				if ((whitesMove && area->board[column + row * 8]->getColor() == TeamColors::eWhite) ||
					(!whitesMove && area->board[column + row * 8]->getColor() == TeamColors::eBlack))
				{
					goto pieceIsSelected;
				}
				//piece can't move(delete outline)
				else
				{
					goto deleteOutline;
				}
			}
			//no piece under cursor(delete outline)
			else
			{
				goto deleteOutline;
			}
		}
		//check if pointed to piece is selected
		else
		{
			goto pieceIsSelected;
		}
	}
	else
	{
		goto deleteOutline;
	}

pieceIsSelected:
	//check if a piece is selected
	if (cursor.selectedPiece)
	{
		//selected piece is not same as pointed to
		if (cursor.selectedPiece->getRow() != row || cursor.selectedPiece->getColumn() != column)
		{
			goto addOutlineOrKeepSame;
		}
		//selected piece is same(delete)
		else
		{
			goto deleteOutline;
		}
	}
	//no piece is selected(check if tile changed again)
	else
	{
		goto addOutlineOrKeepSame;
	}

addOutlineOrKeepSame:
	//add outline
	if (diffTileOrTurnChanged)
	{
		goto addOutline;
	}
	//keep outline same
	else
	{
		return;
	}

deleteOutline:
	cursorOutline = Outline{};
	return;

addOutline:
	cursorOutline = Outline{ vulkan.get(), row, column, OutlineTypes::eCursorHighlight };
	return;
}

Piece::Piece(TeamColors color, int row, int column, PieceTypes type, VulkanResources* vulkan, PlayArea* area)
	: color{ color }, row{ row }, column{ column }, type{ type }, area{ area }, alreadyMoved{ false }
{
	createSprite(vulkan);
}

//swap other with dummy shell
Piece::Piece(Piece&& other) noexcept
	:Piece{}
{
	swap(*this, other);
}

//move construct other and delete it after swapping
Piece& Piece::operator=(Piece other) noexcept
{
	swap(*this, other);

	return *this;
}

void Piece::createSprite(VulkanResources* vulkan)
{
	//pick texture index depending on piece type
	int textureIndex = -1;
	switch (color)
	{
	case TeamColors::eBlack:
		switch (type)
		{
		case PieceTypes::ePawn:
			textureIndex = 2;
			break;
		case PieceTypes::eRook:
			textureIndex = 4;
			break;
		case PieceTypes::eKnight:
			textureIndex = 6;
			break;
		case PieceTypes::eBishop:
			textureIndex = 8;
			break;
		case PieceTypes::eQueen:
			textureIndex = 10;
			break;
		case PieceTypes::eKing:
			textureIndex = 12;
			break;
		default:
			break;
		}
		break;
	case TeamColors::eWhite:
		switch (type)
		{
		case PieceTypes::ePawn:
			textureIndex = 1;
			break;
		case PieceTypes::eRook:
			textureIndex = 3;
			break;
		case PieceTypes::eKnight:
			textureIndex = 5;
			break;
		case PieceTypes::eBishop:
			textureIndex = 7;
			break;
		case PieceTypes::eQueen:
			textureIndex = 9;
			break;
		case PieceTypes::eKing:
			textureIndex = 11;
			break;
		default:
			break;
		}
		break;
	case TeamColors::eRed:
		break;
	case TeamColors::eBlue:
		break;
	default:
		break;
	}

	assert(textureIndex != -1 && "unknown piece being created");
	sprite = GraphicsComponent{ vulkan, -1.0f + 0.25f * column + 1.0f / 8.0f, -1.0f + 0.25f * row + 1.0f / 8.0f, SpriteLayers::eGround, 100.0f / 800.0f, 100.0f / 800.0f,
		0.0f, vulkan->textureSampler, &vulkan->textures[textureIndex] };
}

void Piece::move(int row, int column)
{
	assert(sprite.vulkan && "moved shell piece");

	sprite.moveSprite(-1.0f + 0.25f * column + 1.0f / 8.0f, -1.0f + 0.25f * row + 1.0f / 8.0f);
	this->row = row;
	this->column = column;
}

PlayArea::PlayArea(VulkanResources* vulkan)
	: pieceCount{ 0 }
{
	createSprite(vulkan);
	for (int i = 0; i < board.size(); i++)
	{
		board[i] = nullptr;
	}
	createPieces();
}

void PlayArea::createSprite(VulkanResources* vulkan)
{
	sprite = GraphicsComponent{ vulkan, 0.0f, 0.0f, SpriteLayers::eBackground, 1.0f, 1.0f, 0.0f, vulkan->textureSampler, &vulkan->textures[0] };
}

void PlayArea::createPieces()
{
	for (int i = 0; i < 8; i++)
	{
		placePiece(1, i, TeamColors::eWhite, PieceTypes::ePawn);
	}
	for (int i = 0; i < 8; i++)
	{
		placePiece(6, i, TeamColors::eBlack, PieceTypes::ePawn);
	}
	placePiece(0, 0, TeamColors::eWhite, PieceTypes::eRook);
	placePiece(0, 7, TeamColors::eWhite, PieceTypes::eRook);
	placePiece(7, 0, TeamColors::eBlack, PieceTypes::eRook);
	placePiece(7, 7, TeamColors::eBlack, PieceTypes::eRook);

	placePiece(0, 1, TeamColors::eWhite, PieceTypes::eKnight);
	placePiece(0, 6, TeamColors::eWhite, PieceTypes::eKnight);
	placePiece(7, 1, TeamColors::eBlack, PieceTypes::eKnight);
	placePiece(7, 6, TeamColors::eBlack, PieceTypes::eKnight);

	placePiece(0, 2, TeamColors::eWhite, PieceTypes::eBishop);
	placePiece(0, 5, TeamColors::eWhite, PieceTypes::eBishop);
	placePiece(7, 2, TeamColors::eBlack, PieceTypes::eBishop);
	placePiece(7, 5, TeamColors::eBlack, PieceTypes::eBishop);

	placePiece(0, 3, TeamColors::eWhite, PieceTypes::eQueen);
	placePiece(7, 3, TeamColors::eBlack, PieceTypes::eQueen);

	placePiece(0, 4, TeamColors::eWhite, PieceTypes::eKing);
	placePiece(7, 4, TeamColors::eBlack, PieceTypes::eKing);
}

void PlayArea::placePiece(unsigned char row, unsigned char column, TeamColors color, PieceTypes type)
{
	if (board[column + row * 8])
	{
		throw std::exception("tried placing a piece in an occupied place on the board");
	}

	pieces[pieceCount] = Piece(color, row, column, type, sprite.vulkan, this);
	board[column + row * 8] = &pieces[pieceCount];
	pieceCount++;
}

void PlayArea::removePiece(int location)
{
	*board[location] = Piece();
	board[location] = nullptr;
}