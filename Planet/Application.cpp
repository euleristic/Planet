#include "Application.h"
#include <algorithm>


Application::Application(SDLWrapper::Screen& screen, SDLWrapper::Keyboard& keyboard, SDLWrapper::Mouse& mouse) :
	screen(&screen),
	keyboard(&keyboard),
	mouse(&mouse)
{
	screen.SubscribeToRenderEvent(this);
	keyboard.SubscribeToPressEvent(this);
	mouse.SubscribeToClickEvent(this);
}

Application::~Application() noexcept {
	if (screen)     screen->DetachFromRenderEvent(this);
	if (keyboard) keyboard->DetachFromPressEvent(this);
	if (mouse)       mouse->DetachFromClickEvent(this);
}

bool Application::Update(std::chrono::duration<float> deltaTime) noexcept {

	if (ValidNextVertex(Geometry::ScreenToWorld(SDLWrapper::Mouse::GetPosition()))) {
		lastKnownValidVertex = Geometry::ScreenToWorld(SDLWrapper::Mouse::GetPosition());
	}

	if (path.vertices.size() > 1) {
		float deltaDisplacement = deltaTime.count() * Constants::PLANET_SPEED;
		float distanceToNextVertex = (path.vertices[1] - path.vertices[0]).Magnitude();
		if (deltaDisplacement >= distanceToNextVertex) {
			if (path.vertices.size() == 2) {
				planet = path.vertices.back();
				velocityUnit = {0.0f, 0.0f};
				path.vertices.clear();
			}
			else {
				path.vertices.erase(path.vertices.begin());
				velocityUnit = (path.vertices[1] - path.vertices[0]).Unit();
				planet = path.vertices.front() + velocityUnit * (deltaDisplacement - distanceToNextVertex);
				path.vertices.front() = planet;
			}
		}
		else {
			planet += velocityUnit * deltaDisplacement;
			path.vertices.front() = planet;
		}

	}


	return true;
}

bool Application::ValidNextVertex(const Geometry::Vector2<float> vertex) noexcept {
	if (currentShape.size() == 0) {
		return true;
	}

	// Ensure there is no overlap
	// case line:
	if (Geometry::Intersect(world, { currentShape.back(), vertex })) {
		return false;
	}
	
	// case polygon:
	if (currentShape.size() > 1) {
		Geometry::Polygon polygon{ currentShape };
		polygon.vertices.push_back(vertex);
		for (auto &worldPolygon : world) {
			for (auto &worldVertex : worldPolygon.vertices) {
				if (Geometry::InPolygon(polygon, worldVertex)) {
					return false;
				}
			}
		}
		if (Geometry::InPolygon(polygon, planet)) {
			return false;
		}
		if (!path.vertices.empty() && Geometry::InPolygon(polygon, path.vertices.back())) {
			return false;
		}
	}

	// Ensure potential polygon will be convex.
	if (currentShape.size() > 2) {
		if (direction != Geometry::DirectionOfAngle(*(currentShape.end() - 2), currentShape.back(), vertex)) {
			return false;
		}
		if (direction != Geometry::DirectionOfAngle(currentShape.back(), vertex, currentShape.front())) {
			return false;
		}
		if (direction != Geometry::DirectionOfAngle(vertex, currentShape.front(), *(++currentShape.begin()))) {
			return false;
		}
	}
	return true;
};

bool Application::OnRender(SDLWrapper::Screen::Renderer& renderer) NOEXCEPT_IF_NOT_DEBUG {
	// World
	for (auto it = world.begin(); it != world.end(); ++it) {
		if (!renderer.RenderPolygon(*it, (selectedIndex.has_value() 
			&& (it - world.begin() == selectedIndex) ? Color::PINK : Color::RED))) return false;
	}
	if (currentShape.size() == 1) {
		if (!renderer.RenderLine(Geometry::Line{ currentShape.front(), lastKnownValidVertex }, Color::PINK)) return false;
	}
	if (currentShape.size() > 1) {
		Geometry::Polygon polygon{ currentShape };
		polygon.vertices.push_back(lastKnownValidVertex);
		if (!renderer.RenderPolygon(polygon, Color::PINK)) return false;
	}


	// Entity and path
	if (!path.vertices.empty()) {
		if (!renderer.RenderLineSequence(path, Color::WHITE)) return false;

		// Little goal rendering procedure
		if (!renderer.RenderLine(Geometry::Line{ path.vertices.back() - Geometry::Vector2(-0.05f, -0.05f),
			path.vertices.back() - Geometry::Vector2(0.05f, 0.05f) }, Color::YELLOW)) return false;
		if (!renderer.RenderLine(Geometry::Line{ path.vertices.back() - Geometry::Vector2(0.05f, -0.05f),
			path.vertices.back() - Geometry::Vector2(-0.05f, 0.05f) }, Color::YELLOW)) return false;
	}
	if (!renderer.RenderPoint(planet, Color::GREEN)) return false;
	return true;
}

bool Application::OnKeyPressed(SDLWrapper::Keyboard::KeyCode key) noexcept {
	switch (key) {
	case SDLWrapper::Keyboard::KeyCode::RETURN:
		if (currentShape.size() > 2) {
			Geometry::Polygon polygon{ std::move(currentShape) };
			if (!Geometry::InPolygon(polygon, planet) && (path.vertices.empty() || !Geometry::InPolygon(polygon, path.vertices.back()))) {
				world.push_back(Geometry::Polygon{ polygon });
				if (!path.vertices.empty()) {
					path = AStar::FindPath(world, planet, path.vertices.back());
					velocityUnit = (path.vertices[1] - path.vertices[0]).Unit();
				}
			}
		}
		direction = Geometry::RotationalDirection::UNDEFINED;
		break;

	case SDLWrapper::Keyboard::KeyCode::ESCAPE:
		currentShape.clear();
		direction = Geometry::RotationalDirection::UNDEFINED;
		if (selectedIndex.has_value()) {
			selectedIndex.reset();
		}
		break;

	case SDLWrapper::Keyboard::KeyCode::DELETE:
		if (selectedIndex.has_value()) {
			world.erase(world.begin() + selectedIndex.value());
			selectedIndex.reset();
		}
		break;
	case SDLWrapper::Keyboard::KeyCode::UP:
		AStar::AddThread();
		screen->UpdateTitle(std::to_string(AStar::ThreadCount()) + " threads running A*");
		break;
	case SDLWrapper::Keyboard::KeyCode::DOWN:
		AStar::RemoveThread();
		screen->UpdateTitle(std::to_string(AStar::ThreadCount()) + " threads running A*");
		break;
	default:
		break;
	}
	return true;
}

bool Application::OnMouseClicked(SDLWrapper::Mouse::Button button) noexcept {
	if (button == SDLWrapper::Mouse::Button::LEFT) {
		auto selected = Geometry::InPolygon(world, Geometry::ScreenToWorld(SDLWrapper::Mouse::GetPosition()));
		if (selected != world.end()) {
			selectedIndex = selected - world.begin();
			currentShape.clear();
			direction = Geometry::RotationalDirection::UNDEFINED;
		}
		else {
			selectedIndex.reset();
			if (ValidNextVertex(Geometry::ScreenToWorld(SDLWrapper::Mouse::GetPosition()))) {

				// If we create a triangle, we know which direction vertices are ordered and can store it to enforce it
				if (currentShape.size() == 2) {
					direction = Geometry::DirectionOfAngle(currentShape[0], currentShape[1],
						Geometry::ScreenToWorld(SDLWrapper::Mouse::GetPosition()));
					if (direction == Geometry::RotationalDirection::CLOCKWISE ||
						direction == Geometry::RotationalDirection::COUNTERCLOCKWISE) {
						// Ensure that polygon has area and is valid
						currentShape.push_back(Geometry::ScreenToWorld(SDLWrapper::Mouse::GetPosition()));
					}
				}
				else {
					currentShape.push_back(Geometry::ScreenToWorld(SDLWrapper::Mouse::GetPosition()));
				}
			}
		}
	}

	if (button == SDLWrapper::Mouse::Button::RIGHT) {
		if (Geometry::InPolygon(world, Geometry::ScreenToWorld(SDLWrapper::Mouse::GetPosition())) == world.end()) {
			path = AStar::FindPath(world, planet, Geometry::ScreenToWorld(SDLWrapper::Mouse::GetPosition()));
			velocityUnit = (path.vertices[1] - path.vertices[0]).Unit();
		}
	}
	return true;
}

void Application::OnRendererDestroyed() {
	screen = nullptr;
}

void Application::OnKeyboardDestroyed() {
	keyboard = nullptr;
}

void Application::OnMouseDestroyed() {
	mouse = nullptr;
}

Application::Application(Application&& other) noexcept {
	if (other.screen) {
		other.screen->DetachFromRenderEvent(&other);
		screen = std::exchange(other.screen, nullptr);
		screen->SubscribeToRenderEvent(this);
	}
	if (other.keyboard) {
		other.keyboard->DetachFromPressEvent(&other);
		keyboard = std::exchange(other.keyboard, nullptr);
		keyboard->SubscribeToPressEvent(this);
	}
	if (other.mouse) {
		other.mouse->DetachFromClickEvent(&other);
		mouse = std::exchange(other.mouse, nullptr);
		mouse->DetachFromClickEvent(this);
	}
}

Application& Application::operator=(Application&& other) noexcept{
	if (other.screen) {
		other.screen->DetachFromRenderEvent(&other);
		screen = std::exchange(other.screen, nullptr);
		screen->SubscribeToRenderEvent(this);
	}
	if (other.keyboard) {
		other.keyboard->DetachFromPressEvent(&other);
		keyboard = std::exchange(other.keyboard, nullptr);
		keyboard->SubscribeToPressEvent(this);
	}
	if (other.mouse) {
		other.mouse->DetachFromClickEvent(&other);
		mouse = std::exchange(other.mouse, nullptr);
		mouse->DetachFromClickEvent(this);
	}
	return *this;
}