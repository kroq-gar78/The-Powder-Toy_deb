#include <iostream>

#include "Config.h"
#include <stack>

#include "Global.h"
#include "interface/Window.h"
#include "interface/Platform.h"
#include "interface/Engine.h"
#include "Graphics.h"

using namespace ui;
using namespace std;

Engine::Engine():
	state_(NULL),
	mousex_(0),
	mousey_(0),
	mousexp_(0),
	mouseyp_(0),
	FpsLimit(60.0f),
	windows(stack<Window*>())
{
}

Engine::~Engine()
{
	if(state_ != NULL)
		delete state_;
}

void Engine::Begin(int width, int height)
{
	//engine is now ready
	running_ = true;

	width_ = width;
	height_ = height;
}

void Engine::Exit()
{
	running_ = false;
}

void Engine::ShowWindow(Window * window)
{
	if(state_)
	{
		windows.push(state_);
	}
	state_ = window;
}

void Engine::CloseWindow()
{
	if(!windows.empty())
	{
		state_ = windows.top();
		windows.pop();
	}
	else
	{
		state_ = NULL;
	}
}

/*void Engine::SetState(State * state)
{
	if(state_) //queue if currently in a state
		statequeued_ = state;
	else
	{
		state_ = state;
		if(state_)
			state_->DoInitialized();
	}
}*/

void Engine::SetSize(int width, int height)
{
	width_ = width;
	height_ = height;
}

void Engine::Tick(float dt)
{
	if(state_ != NULL)
		state_->DoTick(dt);

	/*if(statequeued_ != NULL)
	{
		if(state_ != NULL)
		{
			state_->DoExit();
			delete state_;
			state_ = NULL;
		}
		state_ = statequeued_;
		statequeued_ = NULL;

		if(state_ != NULL)
			state_->DoInitialized();
	}*/
}

void Engine::Draw()
{
	if(state_)
		state_->DoDraw();
	g->Blit();
	g->Clear();
}

void Engine::onKeyPress(int key, bool shift, bool ctrl, bool alt)
{
	if(state_)
		state_->DoKeyPress(key, shift, ctrl, alt);
}

void Engine::onKeyRelease(int key, bool shift, bool ctrl, bool alt)
{
	if(state_)
		state_->DoKeyRelease(key, shift, ctrl, alt);
}

void Engine::onMouseClick(int x, int y, unsigned button)
{
	if(state_)
		state_->DoMouseDown(x, y, button);
}

void Engine::onMouseUnclick(int x, int y, unsigned button)
{
	if(state_)
		state_->DoMouseUp(x, y, button);
}

void Engine::onMouseMove(int x, int y)
{
	mousex_ = x;
	mousey_ = y;
	if(state_)
	{
		state_->DoMouseMove(x, y, mousex_ - mousexp_, mousey_ - mouseyp_);
	}
	mousexp_ = x;
	mouseyp_ = y;
}

void Engine::onMouseWheel(int x, int y, int delta)
{
	if(state_)
		state_->DoMouseWheel(x, y, delta);
}

void Engine::onResize(int newWidth, int newHeight)
{
	SetSize(newWidth, newHeight);
}

void Engine::onClose()
{
	if(state_)
		state_->DoExit();
}