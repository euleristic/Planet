#pragma once

#include <cstdint>
#include <variant>

struct Color {
	const uint8_t r, g, b, a;

	constexpr Color(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha) : 
		r(red), g(green), b(blue), a(alpha) { }
	constexpr Color(const uint32_t rgba) : r(static_cast<uint8_t>(rgba >> 24)),
										   g(static_cast<uint8_t>(rgba >> 16)),
										   b(static_cast<uint8_t>(rgba >>  8)),
										   a(static_cast<uint8_t>(rgba)) { }

	static const Color WHITE;
	static const Color BLACK;
	static const Color RED;
	static const Color GREEN;
	static const Color BLUE;
	static const Color YELLOW;
	static const Color CYAN;
	static const Color PINK;
	static const Color ORANGE;
};