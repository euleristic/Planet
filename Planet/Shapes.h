#pragma once

#include "Vector2.h"
#include <vector>

namespace Geometry {

	struct Line {
		Vector2<float> a, b;
	};

	// A sequence of connected lines, which connect at vertices
	struct LineSequence {
		std::vector<Vector2<float>> vertices;
	};

	struct Polygon {
		std::vector<Vector2<float>> vertices;
	};

	// Returns whether lines lhs and rhs intersect.
	[[nodiscard]] bool Intersect(const Line& lhs, const Line& rhs) noexcept;

	// Returns whether polygon and line intersect
	[[nodiscard]] bool Intersect(const Polygon& polygon, const Line& line) NOEXCEPT_IF_NOT_DEBUG;

	//Returns whether line intersects any polygon in world.
	[[nodiscard]] bool Intersect(const std::vector<Polygon>& world, const Line& line) NOEXCEPT_IF_NOT_DEBUG;

	// Returns whether points lies within polygon.
	[[nodiscard]] bool InPolygon(const Polygon& polygon, const Vector2<float>& point) NOEXCEPT_IF_NOT_DEBUG;
		
	// Returns the iterator of the polygon in world that point lies within, or world.end() if none.
	[[nodiscard]] std::vector<Polygon>::const_iterator InPolygon(const std::vector<Polygon>& world, const Vector2<float>& point) NOEXCEPT_IF_NOT_DEBUG;

	// Returns the two vertices in polygon that form the greatest angle with viewPoint.
	// first is the leftmost and second is the rightmost.
	[[nodiscard]] std::pair<Vector2<float>, Vector2<float>> GetAnglularExtrema(const Polygon& polygon, const Vector2<float>& viewPoint) NOEXCEPT_IF_NOT_DEBUG;
}