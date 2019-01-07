#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <SDL.h>
#include "timer.hpp"

const int g_width{640};
const int g_height{480};

const Uint32 color[32]
{
	0x070101FF,
	0x1E0101FF,
	0x3C0100FF,
	0x540100FF,
	0x6B0001FF,
	0x830100FF,
	0x9B0101FF,
	0xB80101FF,
	0xD00001FF,
	0xE70001FF,
	0xFF1C00FF,
	0xFF2F00FF,
	0xFF4100FF,
	0xFF5300FF,
	0xFF6700FF,
	0xFF7900FF,
	0xFF8B01FF,
	0xFF9F01FF,
	0xFFB501FF,
	0xFFC801FF,
	0xFFDA01FF,
	0xFFED00FF,
	0xFFFF00FF,
	0xDBFF00FF,
	0xB3FF00FF,
	0x7AFF00FF,
	0x2EFF00FF,
	0x00FF1EFF,
	0x00FF6BFF,
	0x00FFB7FF,
	0x00FBFFFF,
	0x00AFFFFF
};
Uint32 pixels[g_width * g_height];
std::atomic<bool> g_renderExit{false};
std::atomic<bool> g_fireOn{false};

void render(SDL_Texture *texture, SDL_Renderer *renderer, SDL_Window *window)
{
	Timer timer;
	int fpsCount{0};
	while (!g_renderExit)
	{
		SDL_UpdateTexture(texture, NULL, pixels, g_width * sizeof(Uint32));
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		++fpsCount;

		if (timer.elapsed() > 1.0)
		{
			std::string text{"DOOM Fire"};
			if (g_fireOn)
				text += "[Fire On]    ";
			else
				text += "[Fire Off]    ";
			text += "FPS: ";

			text += std::to_string(std::lround(fpsCount / timer.elapsed()));
			SDL_SetWindowTitle(window, text.c_str());
			timer.reset();
			fpsCount = 0;
		}
	}
}

int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	SDL_Window* window{SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, g_width, g_height, 0)};
	if (!window)
	{
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* renderer{SDL_CreateRenderer(window, -1, 0)};
	if (!renderer)
	{
		std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	SDL_Texture* texture{SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, g_width, g_height)};
	if (!texture)
	{
		std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	std::thread renderThread{render, texture, renderer, window};

	SDL_Event event;
	bool quit{false};

	while (!quit)
	{
		SDL_WaitEvent(&event);
		switch (event.type)
		{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_SPACE:
					g_fireOn = !g_fireOn;
					break;
				}
				break;
		}
	}

	g_renderExit = true;
	renderThread.join();

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
