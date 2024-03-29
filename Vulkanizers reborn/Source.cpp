#include "Game.h"

#include <iostream>

int main()
{
	try
	{
		loadConfig("configs/config.txt");

		Game game;
		
		game.start();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << "\n";
		int n;
		std::cin >> n;
		return -1;
	}

	return 0;
}