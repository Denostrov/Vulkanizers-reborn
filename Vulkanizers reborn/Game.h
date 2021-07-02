#pragma once

#include "VulkanResources.h"
#include "GraphicsComponent.h"
#include "Cursor.h"
#include <random>
#include <array>
#include <cassert>

class PlayArea;

enum class TeamColors
{
	eBlack, eWhite, eRed, eBlue
};

enum class PieceTypes
{
	ePawn, eRook, eKnight, eBishop, eQueen, eKing
};

class Piece
{
public:
	//create shell object
	Piece() = default;
	//create sprite
	Piece(TeamColors color, int row, int column, PieceTypes type, VulkanResources* vulkan, PlayArea* area);

	~Piece() = default;

	Piece(Piece const& other) = delete;	//can't copy pieces

	Piece(Piece&& other) noexcept;		//can move pieces
	Piece& operator=(Piece other) noexcept;

	friend void swap(Piece& first, Piece& second) noexcept
	{
		using std::swap;

		swap(first.sprite, second.sprite);
		swap(first.area, second.area);
		swap(first.type, second.type);
		swap(first.row, second.row);
		swap(first.column, second.column);
		swap(first.color, second.color);
		swap(first.alreadyMoved, second.alreadyMoved);
	}

	void recreateSprite() { createSprite(sprite.vulkan); }	//remake the graphics component
	void move(int row, int column);	//move to cell at row/column

	TeamColors getColor() const { return color; }
	int getRow() const { return row; }
	int getColumn() const { return column; }
	PieceTypes getType() const { return type; }

	bool alreadyMoved;
private:

	void createSprite(VulkanResources* vulkan);	//initialize the graphics component

	GraphicsComponent sprite;
	PlayArea* area;
	PieceTypes type;
	int row;
	int column;
	TeamColors color;
};

class PlayArea
{
public:
	//creates an empty chess board at 0,0
	explicit PlayArea(VulkanResources* vulkan);
	//deletes the board and pieces
	~PlayArea() = default;

	PlayArea(PlayArea const&) = delete;
	PlayArea& operator=(PlayArea const&) = delete;
	PlayArea(PlayArea&&) = delete;
	PlayArea& operator=(PlayArea&&) = delete;

	void recreateSprite() { createSprite(sprite.vulkan); }	//remake the graphics component

	std::array<Piece, 64> pieces;
	std::array<Piece*, 64> board;
	short pieceCount;

	//replace piece at location with shell piece
	void removePiece(int location);
private:
	void createSprite(VulkanResources* vulkan);	//create board sprite
	void createPieces();	//create the default board layout

	//creates the piece together with the sprite
	void placePiece(unsigned char row, unsigned char column, TeamColors color, PieceTypes type);

	GraphicsComponent sprite;
};

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

private:
	void calculateDeltaTime();
	//check window resizing, update cursor/keys
	void processInput();
	void drawFrame();
	void update();
	void resetGame();

	bool keysPressed[512];
	bool keysHeld[512];
	//update key pressed/held arrays
	void updateKeyStates();

	PlayArea* area;
	bool gameOver;
	bool whitesMove;
	//check if who moves changed since last frame
	static bool previousFrameTurnWasWhite;

	std::unique_ptr<SoundEngine> soundEngine;

	std::unique_ptr<VulkanResources> vulkan;

	Cursor cursor;
	std::vector<Outline> highlights;
	bool pieceSelected;

	void selectPiece(Piece* piece, int location);	//update cursor and make highlights
	std::vector<int> getLegalMoveTiles(PieceTypes type, TeamColors color, int location);	//get tiles that piece can move to
	std::vector<int> getLegalCaptureTiles(PieceTypes type, TeamColors color, int location);	//get tiles that piece can capture

	void movePiece(int from, int to);	//move piece into new cell and remove old one there

	bool moveAI();
	std::random_device random;
	std::mt19937 gen;

	//set outline for cursor pointing
	void setCursorOutline();
	Outline cursorOutline;

	Outline checkOutline;
	GLFWwindow* window;
	bool windowResized;

	double lastFrameTime;
	float deltaTime;
	float updateTime;

	int fpsFramesRendered;
	float fpsTimePassed;
};