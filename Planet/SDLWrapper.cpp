#include "SDLWrapper.h"
#include <algorithm>
#include <iterator>
#include <execution>

namespace SDLWrapper {
	int Initialize() NOEXCEPT_IF_NOT_DEBUG {

		// Init everything, return on error, print and throw if _DEBUG is defined
		if (int returnCode = SDL_Init(SDL_INIT_EVERYTHING)) {
			THROW_SDL_ERROR_IF_DEBUG("Could not initialize SDL");
			return returnCode;
		}
		return 0;
	}

	bool Update(Screen& screen, Keyboard& keyboard, Mouse& mouse) noexcept {
		
		// Poll SDL events.
		SDL_Event e;
		while (SDL_PollEvent(&e)) {

			switch (e.type) {

			case SDL_QUIT:
				return false;

			// Notify input observers.

			case SDL_MOUSEBUTTONDOWN:
				if (e.button.button == 1) {
					std::for_each(std::execution::unseq, mouse.observers.begin(), mouse.observers.end(), [](auto& observer) {
						observer->OnMouseClicked(Mouse::Button::LEFT);
					});
				}
				else if (e.button.button == 3) {
					std::for_each(std::execution::unseq, mouse.observers.begin(), mouse.observers.end(), [](auto& observer) {
						observer->OnMouseClicked(Mouse::Button::RIGHT);
					});
				}
				break;
			case SDL_KEYDOWN: {
				Keyboard::KeyCode key = Keyboard::TranslateKeyCode(e.key.keysym.scancode);
				if (key == Keyboard::KeyCode::NOT_SUPPORTED || keyboard.keyMap.test(static_cast<size_t>(key))) {
					break;
				}
				keyboard.keyMap.set(static_cast<size_t>(key));
				std::for_each(std::execution::unseq, keyboard.observers.begin(), keyboard.observers.end(), [key](auto& observer) {
					observer->OnKeyPressed(key);
				});
				break;
			}
			case SDL_KEYUP: {
				Keyboard::KeyCode key = Keyboard::TranslateKeyCode(e.key.keysym.scancode);
				if (key == Keyboard::KeyCode::NOT_SUPPORTED) {
					break;
				}
				keyboard.keyMap.reset(static_cast<size_t>(key));
				break;
			}
			default:
				break;
			}
		}
		return true;
	}

	// Screen

	Screen::Screen() NOEXCEPT_IF_NOT_DEBUG : renderer(window.window) {}

	Screen::~Screen() noexcept {

		// Notify observers
		std::for_each(std::execution::unseq, renderer.observers.begin(), renderer.observers.end(), [](auto& observer) {
			observer->OnRendererDestroyed();
		});
	}

	Screen::Screen(Screen&& other) noexcept :
		window(std::move(other.window)),
		renderer(std::move(other.renderer)) {}

	Screen& Screen::operator=(Screen&& other) noexcept {
		window = std::move(other.window);
		renderer = std::move(other.renderer);
		return *this;
	}

	// Screen::Window

	Screen::Window::Window() NOEXCEPT_IF_NOT_DEBUG : window(SDL_CreateWindow(Constants::APPLICATION_NAME, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, Constants::WINDOW_DIMENSIONS.x, Constants::WINDOW_DIMENSIONS.y, SDL_WINDOW_OPENGL)) {
		if (!window) {
			THROW_SDL_ERROR_IF_DEBUG("Could not create window");
			return;
		}
	}

	Screen::Window::~Window() noexcept {
		SDL_DestroyWindow(window);
	}

	Screen::Window::Window(Window&& other) noexcept : window(std::exchange(other.window, nullptr)) {}

	Screen::Window& Screen::Window::operator=(Window&& other) noexcept {
		window = std::exchange(other.window, nullptr);
		return *this;
	}

	void Screen::Window::SetTitle(std::string_view title) {
		if (!window) {
			THROW_IF_DEBUG("Could not update title: window was nullptr");
			return;
		}
		SDL_SetWindowTitle(window, title.data());
	}

	// Screen::Renderer

	Screen::Renderer::Renderer(SDL_Window* window) NOEXCEPT_IF_NOT_DEBUG : 
		renderer(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)) {
		if (!renderer) {
			THROW_SDL_ERROR_IF_DEBUG("Could not create SDL_Renderer");
			return;
		}
	};

	Screen::Renderer::~Renderer() noexcept {
		SDL_DestroyRenderer(renderer);
	}

	Screen::Renderer::Renderer(Renderer&& other) noexcept : renderer(std::exchange(other.renderer, nullptr)) {}

	Screen::Renderer& Screen::Renderer::operator=(Renderer&& other) noexcept {
		renderer = std::exchange(other.renderer, nullptr);
		return *this;
	}

	bool Screen::Renderer::RenderLineSequence(const Geometry::LineSequence& lines, const Color& color) NOEXCEPT_IF_NOT_DEBUG {
		std::vector<SDL_Point> vertexBuffer;
		vertexBuffer.reserve(lines.vertices.size());
		for (auto& vertex : lines.vertices) {
			auto screenPoint = Geometry::WorldToScreen(vertex);
			vertexBuffer.push_back(SDL_Point{ screenPoint.x, screenPoint.y });
		}
		if ((SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a) ||
			SDL_RenderDrawLines(renderer, vertexBuffer.data(), (int)vertexBuffer.size()))) {
			THROW_SDL_ERROR_IF_DEBUG("Could not render lineSequence");
			return false;
		}
		return true;
	}

	bool Screen::Renderer::RenderPolygon(const Geometry::Polygon& polygon, const Color& color) NOEXCEPT_IF_NOT_DEBUG {

		if (polygon.vertices.size() < 3) {
			THROW_IF_DEBUG("Could not render polygon: too few vertices");
			return false;
		}

		std::vector<SDL_Vertex> vertexBuffer;
		vertexBuffer.reserve(polygon.vertices.size());
		std::ranges::transform(polygon.vertices, std::back_inserter(vertexBuffer), [&](const auto& vertex){ 
			auto screenPoint = Geometry::WorldToScreen(vertex);
			return SDL_Vertex{ {(float)screenPoint.x, (float)screenPoint.y}, {color.r, color.g, color.b, color.a} };
		});
		/*std::ranges::for_each(polygon.vertices, [&](const auto& vertex) {
			vertexBuffer.emplace_back(SDL_Vertex{ {(float)screenPoint.x, (float)screenPoint.y}, {color.r, color.g, color.b, color.a} });
		});*/

		std::vector<int> indexBuffer;
		indexBuffer.reserve(3 * (vertexBuffer.size() - 2));


		for (size_t i = 1; i < vertexBuffer.size() - 1; ++i) { // i is the index of the current triangle's second vertex in vertexBuffer
			indexBuffer.push_back(0);
			indexBuffer.push_back(static_cast<int>(i));
			indexBuffer.push_back(static_cast<int>(i + 1));
		}

		if (SDL_RenderGeometry(renderer, nullptr, vertexBuffer.data(), (int)vertexBuffer.size(),
			indexBuffer.data(), (int)indexBuffer.size())) { // Cast to int because that's what SDL expects. So technically, if you create a polygon that requires INT_MAX / 3 triangles, it will underflow and break. But like, that's on you.
			THROW_SDL_ERROR_IF_DEBUG("Could not render polygon");
			return false;
		}
		return true;
	}

	bool Screen::Renderer::RenderLine(const Geometry::Line& line, const Color& color) NOEXCEPT_IF_NOT_DEBUG {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		const auto screenLineA = Geometry::WorldToScreen(line.a);
		const auto screenLineB = Geometry::WorldToScreen(line.b);
		if (SDL_RenderDrawLine(renderer, screenLineA.x, screenLineA.y, screenLineB.x, screenLineB.y)) {
			THROW_SDL_ERROR_IF_DEBUG("Could not render line");
			return false;
		}
		return true;
	}

	bool Screen::Renderer::RenderPoint(const Geometry::Vector2<float>& point, const Color& color) NOEXCEPT_IF_NOT_DEBUG {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		const auto screenPoint = Geometry::WorldToScreen(point);
		if (SDL_RenderDrawPoint(renderer, screenPoint.x, screenPoint.y)) {
			THROW_SDL_ERROR_IF_DEBUG("Could not render point");
			return false;
		}
		return true;
	}

	bool Screen::Renderer::RenderCurrent() NOEXCEPT_IF_NOT_DEBUG {

		// Clear the screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
		SDL_RenderClear(renderer);

		for (auto& observer : observers) {
			if (!observer->OnRender(*this)) {
				return false;
			};
		}

		SDL_RenderPresent(renderer);
		return true;
	}

	void Screen::SubscribeToRenderEvent(BaseRenderObserver* observer) noexcept {
		renderer.observers.push_back(observer);
	}

	bool Screen::DetachFromRenderEvent(BaseRenderObserver* observer) noexcept {
		auto found = std::find(renderer.observers.begin(), renderer.observers.end(), observer);
		if (found != renderer.observers.end()) {
			renderer.observers.erase(found);
			return true;
		}
		return false;
	}

	bool Screen::RenderCurrent() NOEXCEPT_IF_NOT_DEBUG {
		return renderer.RenderCurrent();
	}

	void Screen::UpdateTitle(const std::string_view suffix) NOEXCEPT_IF_NOT_DEBUG {
		window.SetTitle(std::string(Constants::APPLICATION_NAME) + ": " + suffix.data());
	}

	// Mouse
	[[nodiscard]] Geometry::Vector2<int> Mouse::GetPosition() noexcept {
		int x, y;
		SDL_GetMouseState(&x, &y);
		return { x, y };
	}

	void Mouse::SubscribeToClickEvent(BaseMouseClickObserver* observer) noexcept {
		observers.push_back(observer);
	}

	bool Mouse::DetachFromClickEvent(BaseMouseClickObserver* observer) noexcept {
		auto found = std::find(std::execution::unseq, observers.begin(), observers.end(), observer);
		if (found != observers.end()) {
			observers.erase(found);
			return true;
		}
		return false;
	}

	Mouse::~Mouse() noexcept {
		// Notify observers
		std::for_each(std::execution::unseq, observers.begin(), observers.end(), [](auto& observer) {
			observer->OnMouseDestroyed();
			});
	}

	// Keyboard

	void Keyboard::SubscribeToPressEvent(BaseKeyPressObserver* observer) noexcept {
		observers.push_back(observer);
	}

	bool Keyboard::DetachFromPressEvent(BaseKeyPressObserver* observer) noexcept {
		auto found = std::find(observers.begin(), observers.end(), observer);
		if (found != observers.end()) {
			observers.erase(found);
			return true;
		}
		return false;
	}

	Keyboard::~Keyboard() noexcept {
		// Notify observers
		std::for_each(std::execution::unseq, observers.begin(), observers.end(), [](auto& observer) {
			observer->OnKeyboardDestroyed();
		});
	}

	Keyboard::KeyCode Keyboard::TranslateKeyCode(SDL_Scancode key) noexcept {
		switch (key) {
		case SDL_SCANCODE_ESCAPE: return KeyCode::ESCAPE;
		case SDL_SCANCODE_DELETE: return KeyCode::DELETE;

		case SDL_SCANCODE_0: return KeyCode::K0;
		case SDL_SCANCODE_1: return KeyCode::K1;
		case SDL_SCANCODE_2: return KeyCode::K2;
		case SDL_SCANCODE_3: return KeyCode::K3;
		case SDL_SCANCODE_4: return KeyCode::K4;
		case SDL_SCANCODE_5: return KeyCode::K5;
		case SDL_SCANCODE_6: return KeyCode::K6;
		case SDL_SCANCODE_7: return KeyCode::K7;
		case SDL_SCANCODE_8: return KeyCode::K8;
		case SDL_SCANCODE_9: return KeyCode::K9;

		case SDL_SCANCODE_Q: return KeyCode::Q;
		case SDL_SCANCODE_W: return KeyCode::W;
		case SDL_SCANCODE_E: return KeyCode::E;
		case SDL_SCANCODE_R: return KeyCode::R;
		case SDL_SCANCODE_T: return KeyCode::T;
		case SDL_SCANCODE_Y: return KeyCode::Y;
		case SDL_SCANCODE_U: return KeyCode::U;
		case SDL_SCANCODE_I: return KeyCode::I;
		case SDL_SCANCODE_O: return KeyCode::O;
		case SDL_SCANCODE_P: return KeyCode::P;

		case SDL_SCANCODE_A: return KeyCode::A;
		case SDL_SCANCODE_S: return KeyCode::S;
		case SDL_SCANCODE_D: return KeyCode::D;
		case SDL_SCANCODE_F: return KeyCode::F;
		case SDL_SCANCODE_G: return KeyCode::G;
		case SDL_SCANCODE_H: return KeyCode::H;
		case SDL_SCANCODE_J: return KeyCode::J;
		case SDL_SCANCODE_K: return KeyCode::K;
		case SDL_SCANCODE_L: return KeyCode::L;
		case SDL_SCANCODE_RETURN: return KeyCode::RETURN;

		case SDL_SCANCODE_Z: return KeyCode::Z;
		case SDL_SCANCODE_X: return KeyCode::X;
		case SDL_SCANCODE_C: return KeyCode::C;
		case SDL_SCANCODE_V: return KeyCode::V;
		case SDL_SCANCODE_B: return KeyCode::B;
		case SDL_SCANCODE_N: return KeyCode::N;
		case SDL_SCANCODE_M: return KeyCode::M;

		case SDL_SCANCODE_SPACE: return KeyCode::SPACE;
		case SDL_SCANCODE_LEFT:   return KeyCode::LEFT;
		case SDL_SCANCODE_UP:       return KeyCode::UP;
		case SDL_SCANCODE_DOWN:   return KeyCode::DOWN;
		case SDL_SCANCODE_RIGHT: return KeyCode::RIGHT;
		default: return KeyCode::NOT_SUPPORTED;
		}
	};
}