/*
Copyright (c) 2018 Theta Lin
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <random>
#include <ctime>
#include <SDL.h>
#include "timer.hpp"

const int g_width{512};
const int g_height{256};

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
Uint32 result[g_width * g_height];
std::mutex lock;
std::atomic<bool> g_renderExit{false};
std::atomic<bool> g_fireOn{false};

void spreadFire(size_t src)
{
	static std::mt19937 s_rand{std::random_device{}()};
	const std::uniform_int_distribution<Uint32> distDecay(0, 4);
	const std::uniform_int_distribution<int> distOffset(-1, 1);
	Uint32 decay{distDecay(s_rand) == 0};
	Uint32 dst{src - g_width};
	int offset{distOffset(s_rand)};
	if (offset < 0 && dst == 0) offset = 0;
	pixels[dst + offset] = std::max(decay, pixels[src]) - decay;
}

void doFire()
{
	for (size_t x{0}; x < g_width; ++x)
	{
		for (size_t y{1}; y < g_height; ++y)
		{
			spreadFire(y * g_width + x);
		}
	}
}

void render(SDL_Texture *texture, SDL_Renderer *renderer, SDL_Window *window)
{
	Timer timer;
	int fpsCount{0};
	while (!g_renderExit)
	{
		lock.lock();
		doFire();
		for (size_t i{0}; i < g_height * g_width; ++i)
			result[i] = color[pixels[i]];
		SDL_UpdateTexture(texture, NULL, result, g_width * sizeof(Uint32));
		lock.unlock();

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

		SDL_Delay(20);
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

	memset(pixels, 0, sizeof(pixels));
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
					lock.lock();
					if (g_fireOn)
					{
						for (size_t x{0}; x < g_width; ++x)
							pixels[g_width * (g_height - 1) + x] = 31;
					}
					else
					{
						for (size_t x{0}; x < g_width; ++x)
							pixels[g_width * (g_height - 1) + x] = 0;
					}
					lock.unlock();
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
