#pragma once

#include "SDL.h"
#undef main

#include <vector>
#include <functional>
#include <bitset>
#include "SpaceConversions.h"
#include "Color.h"
#include "Shapes.h"


// Some forward declarations

namespace Geometry {
	struct Polygon;
	struct Line;
	struct LineSequence;
}

// A wrapper for sdl, with a screen containing a window and a renderer, as well as keyboard & mouse inputs.
// Rendering and input is handled through an observer pattern, 
// where classes derived from the base event handlers may subscribe to these events.

namespace SDLWrapper {
	
	int Initialize() NOEXCEPT_IF_NOT_DEBUG;

	// Device declarations for Update function
	class Screen;
	class Keyboard;
	class Mouse;

	bool Update(Screen& screen, Keyboard& keyboard, Mouse& mouse) noexcept;


	// Observer declarations
	class BaseRenderObserver;
	class BaseKeyPressObserver;
	class BaseMouseClickObserver;

	class Screen {
	public:
		class Renderer {
			SDL_Renderer* renderer = nullptr;
			std::vector<BaseRenderObserver*> observers;
			friend Screen;
			bool RenderCurrent() NOEXCEPT_IF_NOT_DEBUG;
		public:
			Renderer() = delete; // A renderer without a window is undefined
			Renderer(SDL_Window* window) NOEXCEPT_IF_NOT_DEBUG;
			~Renderer() noexcept;
			Renderer(Renderer&& other) noexcept;
			Renderer& operator=(Renderer&& other) noexcept;
			Renderer(const Renderer&) = delete;
			Renderer& operator=(const Renderer&) = delete;

			bool RenderPolygon(const Geometry::Polygon& polygon, const Color& color)         NOEXCEPT_IF_NOT_DEBUG;
			bool RenderLineSequence(const Geometry::LineSequence& lines, const Color& color) NOEXCEPT_IF_NOT_DEBUG;
			bool RenderLine(const Geometry::Line& line, const Color& color)                  NOEXCEPT_IF_NOT_DEBUG;
			bool RenderPoint(const Geometry::Vector2<float>& point, const Color& color)    NOEXCEPT_IF_NOT_DEBUG;
		};
	private:

		class Window {
		public:
			SDL_Window* window = nullptr;
			Window() NOEXCEPT_IF_NOT_DEBUG;
			~Window() noexcept;
			Window(Window&& other) noexcept;
			Window& operator=(Window&& other) noexcept;
			Window(const Window&) = delete;
			Window& operator=(const Window&) = delete;
			void SetTitle(std::string_view title);
		} window; 
		Renderer renderer;
		// Order is important! window is contructed before renderer, which is dependant on window.

	public:
		void SubscribeToRenderEvent(BaseRenderObserver* observer) noexcept;
		bool DetachFromRenderEvent(BaseRenderObserver* observer) noexcept;

		bool RenderCurrent() NOEXCEPT_IF_NOT_DEBUG;
		void UpdateTitle(const std::string_view suffix) NOEXCEPT_IF_NOT_DEBUG;

		Screen() NOEXCEPT_IF_NOT_DEBUG;
		~Screen() noexcept;
		Screen(Screen&& other) noexcept;
		Screen& operator=(Screen&& other) noexcept;

		Screen(const Screen&) = delete;
		Screen& operator=(const Screen&) = delete;
	};

	
	class Mouse {

		friend bool Update(Screen&, Keyboard&, Mouse&) noexcept;
		std::vector<BaseMouseClickObserver*> observers;

	public:
		enum class Button { LEFT, RIGHT };
		[[nodiscard]] static Geometry::Vector2<int> GetPosition() noexcept;
		void SubscribeToClickEvent(BaseMouseClickObserver* observer) noexcept;
		bool DetachFromClickEvent(BaseMouseClickObserver* observer) noexcept;

		// Moves can be default. We only care that observers are not notified, and vector's move clears so our destructor is okay.
		Mouse() = default;
		~Mouse() noexcept;
		Mouse(Mouse&& other) noexcept;
		Mouse& operator=(Mouse&& other) noexcept;

		Mouse(const Mouse&) = delete;
		Mouse& operator=(const Mouse&) = delete;
	};

	class Keyboard {
	public:
		enum class KeyCode { 
			NOT_SUPPORTED, ESCAPE, DELETE,
			K1,K2,K3,K4,K5,K6,K7,K8,K9,K0, 
			 Q, W, E, R, T, Y, U, I, O, P,
			  A, S, D, F, G, H, J, K, L, RETURN,
			   Z, X, C, V, B, N, M,
				SPACE, LEFT, UP, DOWN, RIGHT,
			KEYBOARD_SIZE
		};

		void SubscribeToPressEvent(BaseKeyPressObserver* observer) noexcept;
		bool DetachFromPressEvent(BaseKeyPressObserver* observer) noexcept;
	private:
		friend bool Update(Screen&, Keyboard&, Mouse&) noexcept;
		std::vector<BaseKeyPressObserver*> observers;
		static KeyCode TranslateKeyCode(SDL_Scancode key) noexcept;
		std::bitset<(size_t)KeyCode::KEYBOARD_SIZE> keyMap{};
	public:
		// Moves can be default. We only care that observers are not notified, and vector's move clears so our destructor is okay.
		Keyboard() = default;
		~Keyboard() noexcept;
		Keyboard(Keyboard&&) = default;
		Keyboard& operator=(Keyboard&&) = default;

		Keyboard(const Keyboard&) = delete;
		Keyboard& operator=(const Keyboard&) = delete;
	};


	// Observer definitions
	class BaseRenderObserver {
		virtual bool OnRender(Screen::Renderer&) = 0;
		virtual void OnRendererDestroyed() = 0;
		friend Screen;
	};

	class BaseKeyPressObserver {
		virtual bool OnKeyPressed(Keyboard::KeyCode) = 0;
		virtual void OnKeyboardDestroyed() = 0;
		friend bool Update(Screen&, Keyboard&, Mouse&) noexcept;
		friend Keyboard;
	};

	class BaseMouseClickObserver {
		virtual bool OnMouseClicked(Mouse::Button) = 0;
		virtual void OnMouseDestroyed() = 0;
		friend bool Update(Screen&, Keyboard&, Mouse&) noexcept;
		friend Mouse;
	};
}