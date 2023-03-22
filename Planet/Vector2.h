#pragma once
#include <compare>
#include <concepts>
#include "Macros.h"

namespace Geometry {
	template <typename T>
	concept Arithmetic = std::floating_point<T> || std::integral<T>;

	template <Arithmetic ComponentType>
	struct Vector2 {
		ComponentType x, y;

		constexpr Vector2() : x(0), y(0) {};
		constexpr Vector2(const ComponentType x, const ComponentType y) : x(x), y(y) {};

		[[nodiscard]] constexpr Vector2 operator+(const Vector2& other) const noexcept {
			return { x + other.x, y + other.y };
		}

		[[nodiscard]] constexpr Vector2 operator-(const Vector2& other) const noexcept {
			return { x - other.x, y - other.y };
		}

		[[nodiscard]] constexpr Vector2 operator*(const ComponentType scalar) const noexcept {
			return { x * scalar, y * scalar };
		}

		[[nodiscard]] constexpr Vector2 operator/(const ComponentType divisor) const {
			return { x / divisor, y / divisor };
		}

		[[nodiscard]] friend constexpr bool operator==(const Vector2& lhs, const Vector2& rhs) {
			return lhs.x == rhs.x && lhs.y == rhs.y;
		}

		constexpr void operator+=(const Vector2& other) noexcept {
			*this = *this + other;
		}
		constexpr void operator-=(const Vector2& other) noexcept {
			*this = *this - other;
		}
		constexpr void operator*=(const ComponentType scalar) noexcept {
			*this = *this * scalar;
		}
		constexpr void operator/=(const ComponentType divisor) {
			*this = *this / divisor;
		}

		[[nodiscard]] constexpr float MagnitudeSqr() const noexcept {
			return x * x + y * y;
		}

		[[nodiscard]] constexpr float Magnitude() const noexcept {
			return sqrtf(MagnitudeSqr());
		};

		[[nodiscard]] constexpr Vector2 Unit() const noexcept {
			return *this / Magnitude();
		}

		// Returns the counterclockwise normal
		[[nodiscard]] constexpr Vector2 Normal() const noexcept {
			return { -y, x };
		}

		[[nodiscard]] constexpr Vector2 UnitNormal() const noexcept {
			return Normal().Unit();
		}
	};


	template<Arithmetic ComponentType>
	[[nodiscard]] constexpr Vector2<ComponentType> operator*(const ComponentType scalar, const Vector2<ComponentType>& vec) noexcept {
		return vec * scalar;
	}

	// Factor is normalized, so a factor of 0.0f returns from and 1.0f returns toward
	[[nodiscard]] constexpr Vector2<float> NormalizedLerp(const Vector2<float>& from, const Vector2<float>& to, const float factor) noexcept {
		return from + (to - from) * factor;
	}

	// "Raw" means that length is absolute and not normalized
	[[nodiscard]] constexpr Vector2<float> RawLerp(const Vector2<float>& from, const Vector2<float>& to, const float length) noexcept {
		return from + (to - from).Unit() * length;
	}

	template<Arithmetic ComponentType>
	[[nodiscard]] constexpr float Dot(const Vector2<ComponentType>& lhs, const Vector2<ComponentType>& rhs) noexcept {
		return lhs.x * rhs.x + lhs.y * rhs.y;
	}

	template <Arithmetic ComponentType>
	// Returns the Z-component of the cross product, given that the two factor vector are of Z = 0
	[[nodiscard]] constexpr ComponentType CrossZ(const Vector2<ComponentType> lhs, const Vector2<ComponentType> rhs) noexcept {
		return lhs.x * rhs.y - lhs.y * rhs.x;
	}
	
	enum class RotationalDirection {
		CLOCKWISE = 1, STRAIGHT = 0, COUNTERCLOCKWISE = -1, UNDEFINED = 0x80
	};

	// Returns the rotational direction of an angle ABC (in the application spaces, where y+ is down and x+ is right)
	constexpr RotationalDirection DirectionOfAngle(const Vector2<float>& A, const Vector2<float>& B, const Vector2<float>& C) noexcept {
		auto direction = CrossZ(B - A, C - B) <=> 0.0f;
		if (direction == std::partial_ordering::greater)    return RotationalDirection::CLOCKWISE;
		if (direction == std::partial_ordering::equivalent) return RotationalDirection::STRAIGHT;
		if (direction == std::partial_ordering::less)       return RotationalDirection::COUNTERCLOCKWISE;
		return RotationalDirection::UNDEFINED;
	}
}

