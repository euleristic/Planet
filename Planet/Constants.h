#pragma once

#include <string>
#include "Vector2.h"

namespace Constants {

	constexpr const char* APPLICATION_NAME = "PLANET";
	
	// Screen

	// Pixel width and height
	constexpr Geometry::Vector2<int> WINDOW_DIMENSIONS = { 720, 480 };

	// Simply the size of each world basis vector, in pixels 
	constexpr Geometry::Vector2<int> PIXELS_PER_UNIT = { 100, 100 };

	// The origin of world space in world units, from the top left of the window
	constexpr Geometry::Vector2<float> WORLD_ORIGIN = { 
		static_cast<float>(WINDOW_DIMENSIONS.x) * 0.5f / static_cast<float>(PIXELS_PER_UNIT.x), 
		static_cast<float>(WINDOW_DIMENSIONS.y) * 0.5f / static_cast<float>(PIXELS_PER_UNIT.y) 
	};

	// Application

	// How fast the planet traverses its path, in world space units per second
	constexpr float PLANET_SPEED = 1.5f;

	// Geometry

	// Accepted error term to compensate for floating point inaccuracy in intersect predicates
	constexpr float EPSILON = 0.001f;
}