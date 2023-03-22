#include "Shapes.h"
#include "SpaceConversions.h"
#include <algorithm>
#include <ranges>

// This project makes use of the axis of separation theorem, which states that two convex objects do not overlap 
// if you can find an axis onto which their projections are separated.
// I chose to use this because I think it is the cleanest and most intuitive.
// If there exists axes of separation for a given intersection of convex polygons,
// there must be atleast one such axis that is normal to an edge of one of the polygons.
// Therefore it is sufficient to check each such normal.
// 
// A key insight which simplifies and makes more efficient the functions,
// is that an overlap on the projected axis remains even if the projection is scaled.
// Therefore, it is sufficient to dot multiply the shapes with any one normal.

namespace Geometry {

	[[nodiscard]] bool Intersect(const Line& lhs, const Line& rhs) noexcept {
		auto ContainsLineInProjection = [](const Line& toProject, const Line& normalOf) -> bool {
			// By translating the lines to where normalOf.b lands on the origin,
			// it entirely maps onto 0, so we can simply check that the "projected" range contains 0.
			const Line translated = { toProject.a - normalOf.b, toProject.b - normalOf.b };
			const Vector2<float> normal = (normalOf.a - normalOf.b).Normal();

			// Now we project 
			float projectedA = Dot(translated.a, normal);
			float projectedB = Dot(translated.b, normal);

			// Does the range contain 0?
			return signbit(projectedA) != signbit(projectedB);
		};

		if (!ContainsLineInProjection(lhs, rhs)) return false;
		if (!ContainsLineInProjection(rhs, lhs)) return false;
		return true;
	}

	[[nodiscard]] bool InPolygon(const Polygon& polygon, const Vector2<float>& point) NOEXCEPT_IF_NOT_DEBUG {
		if (polygon.vertices.size() < 3) {
			THROW_IF_DEBUG("Function Geometry::InPolygon was passed a polygon of vertieces.size < 3");
			return false;
		}

		// For each edge's normal, does the point map within the range of the polygon's mapping?
		for (auto vertexIt = polygon.vertices.begin(); vertexIt != polygon.vertices.end(); ++vertexIt) {

			// Get the normal of the edge defined by vertex and next(vertex) or front() if next is end.
			const auto normal = ((std::next(vertexIt) != polygon.vertices.end() ?
				*std::next(vertexIt) : polygon.vertices.front()) - *vertexIt).Normal();

			// Get the polygon's minimum and maximum upon projection onto the normal
			const auto [min, max] = std::ranges::minmax(polygon.vertices |
				std::views::transform([&normal](const auto& vertex) { return Dot(vertex, normal); }));

			// Get the point's mapping onto the normal axis
			const auto pointMapping = Dot(point, normal);

			// If there is no overlap, the objects are separated in the normal axis
			if (min > pointMapping || pointMapping > max)
				return false;
		}
		return true;
	}

	[[nodiscard]] bool Intersect(const Polygon& polygon, const Line& line) NOEXCEPT_IF_NOT_DEBUG {

		if (polygon.vertices.size() < 3) {
			THROW_IF_DEBUG("Function Geometry::InPolygon was passed a polygon of vertieces.size < 3");
			return false;
		}

		// For the line's normal, does the range of the polygon's mapping contain the mapping of the line?
		{
			const auto normal = (line.a - line.b).Normal();
			const auto [min, max] = std::ranges::minmax(polygon.vertices |
				std::views::transform([&normal](const auto& vertex) { return Dot(vertex, normal); }));
			const auto lineMapping = Dot(line.b, normal);
			if (min > lineMapping - Constants::EPSILON || lineMapping + Constants::EPSILON > max) return false;
		}

		// For each edge's normal, does the line mapping overlap with the polygon's mapping?
		for (auto vertexIt = polygon.vertices.begin(); vertexIt != polygon.vertices.end(); ++vertexIt) {
			
			// Get the normal of the edge defined by vertex and next(vertex) or front() if next is end.
			const auto normal = ((std::next(vertexIt) != polygon.vertices.end() ?
				*std::next(vertexIt) : polygon.vertices.front()) - *vertexIt).Normal();

			// Get the polygon's minimum and maximum upon projection onto the normal
			const auto [polygonMin, polygonMax] = std::ranges::minmax(polygon.vertices |
				std::views::transform([&normal](const auto& vertex) { return Dot(vertex, normal); }));
			
			// Same goes for line
			const auto [lineMin, lineMax] = std::minmax(Dot(line.a, normal), Dot(line.b, normal));

			// If there is no overlap (given an error of epsilon), the shapes are separated in the normal axis
			if (polygonMin > lineMax - Constants::EPSILON || lineMin + Constants::EPSILON > polygonMax) return false;
		}

		// All axes map an overlap, so the line and polygon must intersect
		return true;
	}

	[[nodiscard]] bool Intersect(const std::vector<Polygon>& world, const Line& line) NOEXCEPT_IF_NOT_DEBUG {
		return !(world | std::views::filter([&line](const auto& polygon) { return Intersect(polygon, line); })).empty();
	}

	[[nodiscard]] std::vector<Polygon>::const_iterator InPolygon(const std::vector<Polygon>& world, const Vector2<float>& point) NOEXCEPT_IF_NOT_DEBUG {
		for (auto polyIt = world.begin(); polyIt != world.end(); polyIt++) {
			if (InPolygon(*polyIt, point)) {
				return polyIt;
			}
		}

		return world.end(); // It's the end of the world! Aah!
	}

	std::pair<Vector2<float>, Vector2<float>> GetAnglularExtrema(const Polygon& polygon, const Vector2<float>& viewPoint) NOEXCEPT_IF_NOT_DEBUG {
		if (InPolygon(polygon, viewPoint)) {
			THROW_IF_DEBUG("Angular extrema undefined, viewPoint was inside polygon");
		}

		auto [leftMost, rightMost] = std::ranges::minmax(polygon.vertices, [&viewPoint](const auto& a, const auto& b) {
			return DirectionOfAngle(a, viewPoint, b) == RotationalDirection::COUNTERCLOCKWISE;
		});

		return std::make_pair(leftMost, rightMost);
	}

}