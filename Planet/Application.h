#pragma once

#include "SDLWrapper.h"
#include "AStar.h"
#include <optional>

class Application final :
	public SDLWrapper::BaseRenderObserver,
	public SDLWrapper::BaseKeyPressObserver,
	public SDLWrapper::BaseMouseClickObserver 
{

	// Store event subjects, if they still exist
	SDLWrapper::Screen*     screen = nullptr;
	SDLWrapper::Keyboard* keyboard = nullptr;
	SDLWrapper::Mouse*       mouse = nullptr;

	// Polygon drawing and interaction
	std::vector<Geometry::Polygon> world;
	std::vector<Geometry::Vector2<float>> currentShape;
	std::optional<ptrdiff_t> selectedIndex;
	Geometry::Vector2<float> lastKnownValidVertex;
	Geometry::RotationalDirection direction = Geometry::RotationalDirection::UNDEFINED;


	// The path finding entity
	Geometry::Vector2<float> planet;
	Geometry::LineSequence path;
	Geometry::Vector2<float> velocityUnit; // Cache the unit velocity so we don't need to do more sqrts than necessary


	// Checks whether the passed vertex is valid. Currentshape should not overlap any polygons in world and should be convex, if it is a polygon.
	bool ValidNextVertex(const Geometry::Vector2<float> vertex) noexcept;

public:
	bool Update(std::chrono::duration<float> deltaTime) noexcept;
	bool OnRender(SDLWrapper::Screen::Renderer& renderer) NOEXCEPT_IF_NOT_DEBUG override;
	bool OnKeyPressed(SDLWrapper::Keyboard::KeyCode key)  noexcept override;
	bool OnMouseClicked(SDLWrapper::Mouse::Button button) noexcept override;

	void OnRendererDestroyed() override;
	void OnKeyboardDestroyed() override;
	void OnMouseDestroyed()    override;

	Application(SDLWrapper::Screen& screen, SDLWrapper::Keyboard& keyboard, SDLWrapper::Mouse& mouse);
	~Application() noexcept;
	Application(Application&& other) noexcept;
	Application& operator=(Application&& other) noexcept;
	Application(const Application&) = delete;
	Application& operator=(const Application) = delete;
};