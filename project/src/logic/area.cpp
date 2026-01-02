#include "logic/area.hpp"

namespace logic
{
	const std::map<Area, const char*> area_names = {
		{Area::Living_room,   "客厅"  },
		{Area::Toilet,        "厕所"  },
		{Area::Kitchen,       "厨房"  },
		{Area::Large_bedroom, "大卧室"},
		{Area::Small_bedroom, "小卧室"},
		{Area::Exterior,      "室外"  },
	};
}