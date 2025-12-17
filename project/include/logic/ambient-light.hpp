#pragma once

#include <glm/glm.hpp>

namespace logic
{
	struct Ambient_light
	{
		float ambient_intensity = 0.03;
		float ao_strength = 1.0f;
		float ao_radius = 70.0f;
		float ao_blend_ratio = 0.017f;

		void control_ui() noexcept;
	};
}