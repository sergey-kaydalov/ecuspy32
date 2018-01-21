/*
 * config.cpp
 *
 *  Created on: Jan 21, 2018
 *      Author: ksu
 */
#include <array>
#include <vector>
#include <iostream>
#include <algorithm>

namespace ecuspy {

enum ConfigCat_t{
	cfgCatWIFI,
	cfgCatELM327
};

enum ConfigValueType_t{
	cfgTypeBOOL,
	cfgTypeINT,
	cfgTypeString,
	cfgTypeDouble
};


struct ConfigEntry {

	constexpr ConfigEntry(const char* _id, const char* _name, const char* _desc,
		ConfigCat_t _cat, ConfigValueType_t _type, size_t _len)
	: id(_id),name(_name),desc(_desc), cat(_cat), type(_type), len(_len)
	{}

	const char* id;
	const char* name;
	const char* desc;
	ConfigCat_t cat;
	ConfigValueType_t type;
//	const ConfigEntry* dep;
//	const char* depValue;
	size_t offset = 0;
	size_t len = 0;
};


class Config {
public:
	template <typename... Tis>
	constexpr Config(Tis... args) : cfg{args...} {
		auto it = cfg.begin();
		auto prev = it;
		auto end = cfg.end();

		for (; it != end; it++) {
			if (it != prev)
				it->offset = prev->len + prev->offset;
		}
	}

	std::vector<ConfigEntry> cfg;
};


const Config cfg(ConfigEntry("id1", "name1", "desc1", cfgCatWIFI, cfgTypeString, 10),
		ConfigEntry("id2", "name2Str", "desc2", cfgCatWIFI, cfgTypeString, 15),
		ConfigEntry("id3", "name3Int", "desc3", cfgCatWIFI, cfgTypeINT, 15),
		ConfigEntry("id4", "name4Bool", "desc4", cfgCatWIFI, cfgTypeBOOL, 15),
		ConfigEntry("id5", "name5Double", "desc5", cfgCatWIFI, cfgTypeDouble, 15),
		ConfigEntry("id6", "name6", "desc6", cfgCatWIFI, cfgTypeString, 15)
		);

}



