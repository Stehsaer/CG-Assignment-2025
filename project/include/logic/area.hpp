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

	///
	/// @brief Mappings from Area enum to area names
	///
	///
	extern const std::map<Area, const char*> area_names;
}
