// C++20 or newer

#include "Application.h"

// If in debug mode, the application is wrapped in a try/catch block.
// It is mostly noexcept if in release mode, so that it runs faster.

int main() {

	TRY_IF_DEBUG;

	// Initialize everything

	SDLWrapper::Initialize();
	SDLWrapper::Screen screen;
	SDLWrapper::Keyboard keyboard;
	SDLWrapper::Mouse mouse;
	Application application(screen, keyboard, mouse);

	auto currentTime = std::chrono::steady_clock::now();
	// Program loop. Update SDL, handle input and render.
	while (true) {

		if (!SDLWrapper::Update(screen, keyboard, mouse)) {
			break;
		};

		auto deltaTime = std::chrono::steady_clock::now() - currentTime;
		currentTime = std::chrono::steady_clock::now();

		if (!application.Update(deltaTime))
			break;

		if (!screen.RenderCurrent())
			break;

	}

	CATCH_IF_DEBUG;

	return 0;
}