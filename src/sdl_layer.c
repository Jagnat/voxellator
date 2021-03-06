#include "platform.h"

#include <SDL2/SDL.h>

#include <GL/gl.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gldefs.h"

typedef struct
{
	SDL_Window *window;
	SDL_GLContext glcontext;
	double timerResolution;

	bool mouseLocked;
} SdlData;

static SdlData _sdl = {};
static SdlData *sdl = &_sdl;

static PlatformState _sdl_platform = {};
static PlatformState *sdl_platform = &_sdl_platform;

static void sdl_loadGlFuncs();
static void sdl_handleEvents();
static void sdl_centerCursor();

int main(int argc, char **argv)
{
	sdl->timerResolution = (double)SDL_GetPerformanceFrequency() / 1000.L;

	sdl_platform->running = true;
	sdl_platform->updateTarget = 1000.L / 120.L;
	sdl_platform->renderTarget = 1000.L / 60.L;
	sdl_platform->viewportWidth = 1280;
	sdl_platform->viewportHeight = 720;

	sdl_platform->info.logicalCores = SDL_GetCPUCount();

	sdl->window = SDL_CreateWindow("Voxellator",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		sdl_platform->viewportWidth, sdl_platform->viewportHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	if (!sdl->window)
		return 1;

	sdl->glcontext = SDL_GL_CreateContext(sdl->window);
	if (!sdl->glcontext)
		return 1;

	sdl_loadGlFuncs();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(sdl->window);

	init(sdl_platform);

	double prevTime, currentTime, elapsedTime, updateDelta, renderDelta;
	elapsedTime = updateDelta = renderDelta = 0;

	prevTime = get_elapsed_ms();

	while (sdl_platform->running)
	{
		sdl_handleEvents();

		if (getPlatformFlag(MOUSE_LOCKED))
			sdl_centerCursor();

		currentTime = get_elapsed_ms();

		elapsedTime = currentTime - prevTime;

		if (elapsedTime > 100.L)
		{
			// TODO: Diagnose
			printf("Running slow!\n");
			elapsedTime = 100;
		}

		prevTime = currentTime;

		updateDelta += elapsedTime;
		renderDelta += elapsedTime;

		while (updateDelta >= sdl_platform->updateTarget)
		{
			update();
			sdl_platform->filledEvents = 0;
			updateDelta -= sdl_platform->updateTarget;
		}

		if (renderDelta >= sdl_platform->renderTarget)
		{
			render(updateDelta / sdl_platform->updateTarget);
			SDL_GL_SwapWindow(sdl->window);
			renderDelta = 0;
		}
	}

	return 0;
}

void sdl_setPlatformFlag(int flag, int state)
{
	if (state)
		sdl_platform->flags |= flag;
	else
		sdl_platform->flags &= ~flag;
}

int getPlatformFlag(int flag) { return !!(sdl_platform->flags & flag); }

void sdl_centerCursor()
{
	SDL_WarpMouseInWindow(sdl->window,
		sdl_platform->viewportWidth / 2, sdl_platform->viewportHeight / 2);
}

void setMouseState(int locked)
{
	if(locked == getPlatformFlag(MOUSE_LOCKED))
		return;
	
	if (locked)
	{
		sdl_setPlatformFlag(MOUSE_LOCKED, true);
		sdl_centerCursor();
		SDL_ShowCursor(SDL_DISABLE);
	}
	else
	{
		sdl_setPlatformFlag(MOUSE_LOCKED, false);
		SDL_ShowCursor(SDL_ENABLE);
	}
}

void sdl_postEvent(Event event)
{
	assert(sdl_platform->filledEvents < EVENT_QUEUE_SIZE);
	sdl_platform->eventQueue[sdl_platform->filledEvents++] = event;
}

static void sdl_handleEvents()
{
	Event post;

	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		Event post;
		switch(e.type)
		{
			case SDL_QUIT:
			{
				sdl_platform->running = false;
			}
			break;

			case SDL_WINDOWEVENT:
			{
				switch(e.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
					{
						printf("resize\n");
						post.type = EVENT_RESIZE;
						post.resize.width = e.window.data1;
						post.resize.height = e.window.data2;
						sdl_platform->viewportWidth = post.resize.width;
						sdl_platform->viewportHeight = post.resize.height;
						sdl_postEvent(post);
					}
					break;
				}
			}
			break;

			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				if (e.key.keysym.sym == SDLK_ESCAPE)
					sdl_platform->running = false;

				post.type = EVENT_KEY;
				post.key.keyCode = e.key.keysym.sym;
				if (e.key.keysym.sym == SDLK_LSHIFT)
					post.key.keyCode = 0xa0; // TODO: FIX THIS STUPID HACK
				post.key.state = e.key.state == SDL_PRESSED? BUTTON_PRESSED : BUTTON_RELEASED;
				sdl_postEvent(post);
			}
			break;

			case SDL_MOUSEMOTION:
			{
				post.type = EVENT_MOUSE_MOVE;
				post.mouseMove.x = e.motion.x;
				post.mouseMove.y = e.motion.y;
				
				if (getPlatformFlag(MOUSE_LOCKED))
				{
					int cX = sdl_platform->viewportWidth / 2;
					int cY = sdl_platform->viewportHeight / 2;
					post.mouseMove.locked = true;
					post.mouseMove.dx = e.motion.x - cX;
					post.mouseMove.dy = e.motion.y - cY;
				}

				sdl_postEvent(post);
			}
			break;
		}
	}
}

static void sdl_loadGlFuncs()
{
	#define GLDEF(ret, name, ...) gl##name = \
		(name##proc *) SDL_GL_GetProcAddress("gl" #name);
	GL_LIST
	#undef GLDEF

	// Ensure functions have successfully loaded
	// TODO: something more robust for release mode
	#define GLDEF(retrn, name, ...) assert(gl##name);
	GL_LIST
	#undef GLDEF
}

int createThread(void (*threadProc)(void*), void *threadArgs)
{
	SDL_Thread *t=SDL_CreateThread(threadProc, "test_name", threadArgs);
	if (!t)
		return false;
	SDL_DetachThread(t);
	return true;
}

void atomicIncrement(volatile int *val)
{
	SDL_AtomicIncRef((SDL_atomic_t*)val);
}

void atomicDecrement(volatile int *val)
{
	SDL_AtomicDecRef((SDL_atomic_t*)val);
}

void *createMutex()
{
	return SDL_CreateMutex();
}

int lockMutex(void *mutex)
{
	return SDL_LockMutex((SDL_mutex*)mutex);
}

int unlockMutex(void *mutex)
{
	return SDL_UnlockMutex((SDL_mutex*)mutex);
}

double get_elapsed_ms()
{
	return (double)SDL_GetPerformanceCounter() / sdl->timerResolution;
}

void sleepMs(int ms)
{
	SDL_Delay(ms);
}

