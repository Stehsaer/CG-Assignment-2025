#pragma once

#include <map>

namespace logic
{
	enum class Area
	{
		Living_room,
		Toilet,
		Kitchen,
		Large_bedroom,
		Small_bedroom,
		Exterior
	};

	extern const std::map<Area, const char*> area_names;
}
