#pragma once

namespace Hush::MathUtils {

	inline float Clamp(float value, float min, float max) {
		const float t = value < min ? min : value;
		return t > max ? max : t;
	}

	inline float Lerp(float from, float to, float t) {
		const float tClamped = Clamp(t, 0.0f, 1.0f);
		return from + (to - from) * tClamped;
	}

	inline float Pow(float x, float n) {
		return pow(x, n);
	}
}
