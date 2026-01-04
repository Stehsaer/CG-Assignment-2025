#pragma once

#include <cstdint>

namespace render
{
	constexpr float REF_LUMINANCE = 500.0f;

	constexpr uint32_t SHADOW_LEVEL_RES_0 = 3072;
	constexpr uint32_t SHADOW_LEVEL_RES_1 = 1536;
	constexpr uint32_t SHADOW_LEVEL_RES_2 = 1024;

	constexpr float EXPOSURE_MIN = 1e-2;
	constexpr float EXPOSURE_MAX = 500.0f;
	constexpr float EXPOSURE_EYE_ADAPTATION_RATE = 1.5f;

	constexpr float BLOOM_START_THRES = 2.0f;
	constexpr float BLOOM_END_THRES = 10.0f;
}