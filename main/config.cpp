/*
 * config.cpp
 *
 *  Created on: Jan 21, 2018
 *      Author: ksu
 */
#include "config.hpp"

namespace ecuspy {

//enum ConfigCat_t{
//	cfgCatWIFI,
//	cfgCatELM327
//};
//
//enum ConfigValueType_t{
//	cfgTypeBOOL,
//	cfgTypeINT,
//	cfgTypeString,
//	cfgTypeDouble
//};
//
//
//struct ConfigEntry {
//
//	constexpr ConfigEntry(const char* _id, const char* _name, const char* _desc,
//		ConfigCat_t _cat, ConfigValueType_t _type, size_t _len)
//	: id(_id),name(_name),desc(_desc), cat(_cat), type(_type), len(_len)
//	{}
//
//	const char* id;
//	const char* name;
//	const char* desc;
//	ConfigCat_t cat;
//	ConfigValueType_t type;
//	size_t offset = 0;
//	size_t len = 0;
//};
//
//
//template <unsigned N>
//class Config {
//public:
//	template <typename... Tis>
//	constexpr Config(Tis... args) : cfg{{args...}} {
//		auto it = cfg.begin();
//		auto prev = it;
//		auto end = cfg.end();
//
//		for (; it != end; it++) {
//			if (it != prev)
//				it->offset = prev->len + prev->offset;
//		}
//	}
//
//	constexpr ConfigEntry& get(size_t index) { return cfg[index]; };
//	std::array<ConfigEntry, N> cfg;
//};

//template <unsigned N>
//const ConfigEntry& Config<N>::get(size_t index) const {
//	return cfg[index];
//}

}





