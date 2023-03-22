#pragma once
#include "Constants.h"

namespace Geometry {
	[[nodiscard]] constexpr Vector2<float> ScreenToWorld(const Vector2<int>& screenCoords) {
		const Vector2<float> scaled = { (float)screenCoords.x / (float)Constants::PIXELS_PER_UNIT.x,
										(float)screenCoords.y / (float)Constants::PIXELS_PER_UNIT.y };
		return (scaled - Constants::WORLD_ORIGIN);
	};

	[[nodiscard]] constexpr Vector2<int> WorldToScreen(const Vector2<float>& position) {
		const Vector2<float> translated = position + Constants::WORLD_ORIGIN;
		return { (int)(translated.x * (float)Constants::PIXELS_PER_UNIT.x),
			(int)(translated.y * (float)Constants::PIXELS_PER_UNIT.y) };
	};
}